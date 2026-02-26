
#include <pch.hpp>

class ThreadSafeModel; // Forward declaration

class GLContextWorker {
public:
	explicit GLContextWorker(GLFWwindow* sharedWindow) {
		if (!sharedWindow) {
			throw std::invalid_argument("GLContextWorker: sharedWindow cannot be null");
		}

		std::cout << "[GLContextWorker] Creating shared context..." << std::endl;



		// Store original hints
		int originalVisible = 1, originalVersionMajor = 4, originalVersionMinor = 6, originalProfile = GLFW_OPENGL_CORE_PROFILE;
		//  glfwGetWindowHint(GLFW_VISIBLE, &originalVisible);
		//  glfwGetWindowHint(GLFW_CONTEXT_VERSION_MAJOR, &originalVersionMajor);
		//  glfwGetWindowHint(GLFW_CONTEXT_VERSION_MINOR, &originalVersionMinor);
		//  glfwGetWindowHint(GLFW_OPENGL_PROFILE, &originalProfile);

		try {
			// Set up context creation hints matching the shared window
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

			workerWindow = glfwCreateWindow(1, 1, "GPU Worker", nullptr, sharedWindow);
			if (!workerWindow) {
				const char* errorDesc;
				int errorCode = glfwGetError(&errorDesc);
				std::stringstream ss;
				ss << "Failed to create GPU worker context (Error " << errorCode << ": " << errorDesc << ")";
				throw std::runtime_error(ss.str());
			}

			std::cout << "[GLContextWorker] Worker context created successfully" << std::endl;

			// Verify context sharing
			if (!VerifyContextSharing(sharedWindow)) {
				throw std::runtime_error("Context sharing verification failed - OpenGL objects will not be shared between contexts");
			}

			// Start worker thread
			workerThread = std::thread(&GLContextWorker::ThreadLoop, this);
			workerThreadId = workerThread.get_id();

			// Wait for worker thread to initialize
			WaitForInitialization();

		}
		catch (...) {
			// Restore original hints
			glfwWindowHint(GLFW_VISIBLE, originalVisible);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, originalVersionMajor);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, originalVersionMinor);
			glfwWindowHint(GLFW_OPENGL_PROFILE, originalProfile);
			throw;
		}

		// Restore original hints
		glfwWindowHint(GLFW_VISIBLE, originalVisible);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, originalVersionMajor);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, originalVersionMinor);
		glfwWindowHint(GLFW_OPENGL_PROFILE, originalProfile);
	}

	~GLContextWorker() {
		Stop();
		if (workerThread.joinable()) {
			workerThread.join();
		}
		if (workerWindow) {
			glfwDestroyWindow(workerWindow);
		}
		std::cout << "[GLContextWorker] Destroyed" << std::endl;
	}

	// Non-copyable, non-movable
	GLContextWorker(const GLContextWorker&) = delete;
	GLContextWorker& operator=(const GLContextWorker&) = delete;
	GLContextWorker(GLContextWorker&&) = delete;
	GLContextWorker& operator=(GLContextWorker&&) = delete;

	// Add a GPU job to the worker
	void AddJob(std::function<void()> job) {
		if (!job) {
			std::cerr << "[GLContextWorker] Warning: Attempted to add null job" << std::endl;
			return;
		}

		{
			std::lock_guard<std::mutex> lock(queueMutex);
			if (shouldExit) {
				std::cerr << "[GLContextWorker] Warning: Adding job after shutdown requested" << std::endl;
				return;
			}
			renderQueue.push(std::move(job));
			queueSize = renderQueue.size();
		}
		cv.notify_one();
	}

	// Async model loading
	void AsyncLoadModel(std::shared_ptr<ThreadSafeModel> modelWrapper, const std::string& path) {
		if (!modelWrapper) {
			std::cerr << "[GLContextWorker] Error: modelWrapper is null in AsyncLoadModel" << std::endl;
			return;
		}

		if (path.empty()) {
			std::cerr << "[GLContextWorker] Error: Empty path provided for model loading" << std::endl;
			return;
		}

		std::cout << "[GLContextWorker] Starting async model load: " << path << std::endl;

		// Start CPU loading in separate thread
		std::thread loaderThread([this, modelWrapper, path]() {
			try {
				std::cout << "[GLContextWorker] CPU loading started on thread "
					<< std::this_thread::get_id() << std::endl;

				std::cout << "[GLContextWorker] CPU work completed, queuing GPU upload..." << std::endl;

				// Queue GPU upload on worker thread
				this->AddJob([modelWrapper, path]() {
					std::cout << "[GLContextWorker] GPU upload starting on thread "
						<< std::this_thread::get_id() << std::endl;

					GLenum error = glGetError();
					if (error != GL_NO_ERROR) {
						std::cerr << "[GLContextWorker] OpenGL error before model creation: " << error << std::endl;
					}

					try {
						auto start = std::chrono::steady_clock::now();
						std::unique_ptr<Model> newModel = std::make_unique<Model>(path);

						error = glGetError();
						if (error != GL_NO_ERROR) {
							std::cerr << "[GLContextWorker] OpenGL error during model creation: " << error << std::endl;
						}

						modelWrapper->SetModel(std::move(newModel));
						std::cout << "[GLContextWorker] Async mesh uploaded successfully!" << std::endl;
						auto end = std::chrono::steady_clock::now();

						modelWrapper->m_LoadTime = std::chrono::duration<double>(end - start);
						if (std::filesystem::exists(path)) {
							modelWrapper->m_FileSize= std::filesystem::file_size(path)/1024/1024;
						}
						modelWrapper->GenerateThumbnail();
					}
					catch (const std::exception& e) {
						std::cerr << "[GLContextWorker] Failed to load model asynchronously: " << e.what() << std::endl;
						modelWrapper->Reset();
					}
					catch (std::filesystem::filesystem_error& e)
					{
						std::cerr << e.what() << std::endl;
					}

					error = glGetError();
					if (error != GL_NO_ERROR) {
						std::cerr << "[GLContextWorker] OpenGL error after async load: " << error << std::endl;
					}
					});

			}
			catch (const std::exception& e) {
				std::cerr << "[GLContextWorker] Exception in loader thread: " << e.what() << std::endl;
			}
			});

		loaderThread.detach();
	}

	// Stop the worker thread
	void Stop() {
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			shouldExit = true;
			queueSize = 0;
		}
		cv.notify_all();
	}

	// Getters for debugging
	GLFWwindow* GetWorkerWindow() const { return workerWindow; }
	std::thread::id GetThreadId() const { return workerThreadId; }
	size_t GetQueueSize() const { return queueSize.load(); }
	bool IsRunning() const { return !shouldExit && workerThread.joinable(); }

private:
	GLFWwindow* workerWindow = nullptr;
	std::thread workerThread;
	std::thread::id workerThreadId;

	std::queue<std::function<void()>> renderQueue;
	std::mutex queueMutex;
	std::condition_variable cv;
	std::atomic<bool> shouldExit{ false };
	std::atomic<size_t> queueSize{ 0 };

	// Initialization synchronization
	std::mutex initMutex;
	std::condition_variable initCV;
	bool workerInitialized = false;

	bool VerifyContextSharing(GLFWwindow* mainWindow) {
		std::cout << "[GLContextWorker] Verifying context sharing..." << std::endl;

		// Test 1: Check if contexts report the same OpenGL version
		glfwMakeContextCurrent(mainWindow);
		const char* mainVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const char* mainVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

		glfwMakeContextCurrent(workerWindow);
		const char* workerVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const char* workerVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

		glfwMakeContextCurrent(mainWindow); // Restore main context

		bool versionsMatch = (mainVersion && workerVersion && std::string(mainVersion) == std::string(workerVersion));
		bool vendorsMatch = (mainVendor && workerVendor && std::string(mainVendor) == std::string(workerVendor));

		std::cout << "[GLContextWorker] Main Context: " << (mainVendor ? mainVendor : "Unknown")
			<< " - " << (mainVersion ? mainVersion : "Unknown") << std::endl;
		std::cout << "[GLContextWorker] Worker Context: " << (workerVendor ? workerVendor : "Unknown")
			<< " - " << (workerVersion ? workerVersion : "Unknown") << std::endl;

		if (!versionsMatch || !vendorsMatch) {
			std::cerr << "[GLContextWorker] Warning: Context version/vendor mismatch - sharing may not work properly" << std::endl;
			return false;
		}

		std::cout << "[GLContextWorker] Context sharing verified successfully" << std::endl;
		return true;
	}

	void WaitForInitialization() {
		std::unique_lock<std::mutex> lock(initMutex);
		if (!initCV.wait_for(lock, std::chrono::seconds(5), [this] { return workerInitialized; })) {
			throw std::runtime_error("GPU worker thread failed to initialize within timeout");
		}
	}

	void SignalInitialization() {
		{
			std::lock_guard<std::mutex> lock(initMutex);
			workerInitialized = true;
		}
		initCV.notify_all();
	}

	void ThreadLoop() {
		try {
			// Make worker context current
			glfwMakeContextCurrent(workerWindow);

			// Initialize GLAD for this context (CRITICAL!)
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				throw std::runtime_error("Failed to initialize GLAD in worker thread");
			}

			// Verify OpenGL functionality
			GLuint testVAO;
			glGenVertexArrays(1, &testVAO);
			GLenum error = glGetError();
			if (error != GL_NO_ERROR || testVAO == 0) {
				throw std::runtime_error("OpenGL functionality test failed in worker thread");
			}
			glDeleteVertexArrays(1, &testVAO);

			// Set up OpenGL state
			glEnable(GL_DEPTH_TEST);

			std::cout << "[GLContextWorker] Worker thread initialized successfully - OpenGL "
				<< GLVersion.major << "." << GLVersion.minor
				<< " (" << glGetString(GL_VERSION) << ")" << std::endl;

			// Signal that we're ready
			SignalInitialization();

			// Main loop
			while (!shouldExit && !glfwWindowShouldClose(workerWindow)) {
				std::function<void()> job;

				// Wait for job or shutdown
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					cv.wait(lock, [this] {
						return !renderQueue.empty() || shouldExit;
						});

					if (shouldExit && renderQueue.empty()) break;

					if (!renderQueue.empty()) {
						job = std::move(renderQueue.front());
						renderQueue.pop();
						queueSize = renderQueue.size();
					}
				}

				// Execute job
				if (job) {
					try {
						job();
					}
					catch (const std::exception& e) {
						std::cerr << "[GLContextWorker] Exception in job execution: " << e.what() << std::endl;
					}
				}

				// Small yield to prevent busy waiting
				std::this_thread::sleep_for(std::chrono::microseconds(100));
			}

		}
		catch (const std::exception& e) {
			std::cerr << "[GLContextWorker] Fatal error in worker thread: " << e.what() << std::endl;
			SignalInitialization(); // Still signal to avoid deadlock
			return;
		}

		std::cout << "[GLContextWorker] Worker thread exiting" << std::endl;
	}
};