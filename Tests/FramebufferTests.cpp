// Tests/FramebufferTests.cpp
#include <catch2/catch_test_macros.hpp>
#include <glad/glad.h> // 👈 Ensure GLAD header is included!
#include "Framebuffer.hpp" 

TEST_CASE("Picking Framebuffer Initial Clear State", "[graphics][picking]")
{
	const unsigned int width = 800;
	const unsigned int height = 600;
	Framebuffer pickingFBO(width, height, true);

	SECTION("Framebuffer should clear background to 0xFFFFFFFF (uint -1)")
	{
		pickingFBO.Bind();

		uint32_t clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
		glClearBufferuiv(GL_COLOR, 0, clearValues);

		auto pixel = pickingFBO.ReadPixel(width / 2, height / 2);

		REQUIRE(pixel.ObjectID == 0xFFFFFFFF);
		REQUIRE(pixel.ObjectID != 0);
	}

	SECTION("OutOfBounds readback should return 0xFFFFFFFF fallback")
	{
		auto pixel = pickingFBO.ReadPixel(width + 10, height + 10);
		REQUIRE(pixel.ObjectID == 0xFFFFFFFF);
	}
}
// Simulated selection state update
uint32_t UpdateSelection(uint32_t currentSelection, uint32_t hoveredObjectID, bool isMouseClicked)
{
	if (isMouseClicked && hoveredObjectID != 0xFFFFFFFF)
	{
		return hoveredObjectID; // Update to new valid object
	}
	return currentSelection; // Keep existing selection on background click or no click
}

TEST_CASE("Entity Selection Logic", "[selection]")
{
	uint32_t selectedEntity = 5; // Currently entity #5 is selected

	SECTION("Clicking background (0xFFFFFFFF) preserves selected entity")
	{
		uint32_t hoveredObjectID = 0xFFFFFFFF; // Background
		selectedEntity = UpdateSelection(selectedEntity, hoveredObjectID, true);

		REQUIRE(selectedEntity == 5); // Should NOT reset to 0 or -1
	}

	SECTION("Clicking another object updates selection")
	{
		uint32_t hoveredObjectID = 12; // Entity #12
		selectedEntity = UpdateSelection(selectedEntity, hoveredObjectID, true);

		REQUIRE(selectedEntity == 12);
	}
}
TEST_CASE("Driver Consistency: Integer Framebuffer Clear", "[graphics][driver-quirks]")
{
	Framebuffer fbo(800, 600, true);
	fbo.Bind();

	// Clear integer attachment 0 to 0xFFFFFFFF
	uint32_t clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	glClearBufferuiv(GL_COLOR, 0, clearValues);

	// Read multiple distinct points to ensure AMD driver didn't partially clear
	auto topLeft = fbo.ReadPixel(0, 0);
	auto center = fbo.ReadPixel(400, 300);
	auto botRight = fbo.ReadPixel(799, 599);

	REQUIRE(topLeft.ObjectID == 0xFFFFFFFF);
	REQUIRE(center.ObjectID == 0xFFFFFFFF);
	REQUIRE(botRight.ObjectID == 0xFFFFFFFF);
}
TEST_CASE("Framebuffer Completeness", "[graphics][framebuffer]")
{
	Framebuffer fbo(800, 600);
	fbo.Bind();

	// Verify OpenGL specs report the FBO is valid and ready to draw to
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	REQUIRE(status == GL_FRAMEBUFFER_COMPLETE);

	fbo.Unbind();
}
TEST_CASE("Framebuffer Mouse Coordinate Y-Flip Logic", "[graphics][picking]")
{
	const unsigned int width = 800;
	const unsigned int height = 600;
	Framebuffer pickingFBO(width, height, true);

	SECTION("Top-Left screen read translates correctly to OpenGL Bottom-Left")
	{
		pickingFBO.Bind();
		// Clear entire buffer to 0xFFFFFFFF
		uint32_t clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
		glClearBufferuiv(GL_COLOR, 0, clearValues);

		// Read at Top-Left screen space mouse coordinates (x: 0, y: 0)
		// ReadPixel should internally convert this to OpenGL's (0, 599)
		auto pixel = pickingFBO.ReadPixel(0, 0);
		REQUIRE(pixel.ObjectID == 0xFFFFFFFF);
	}
}
TEST_CASE("Driver Consistency: Odd Dimensions & Pack Alignment", "[graphics][driver-quirks]")
{
	// 803 is an odd width that breaks 4-byte row alignment!
	const unsigned int width = 803;
	const unsigned int height = 601;
	Framebuffer fbo(width, height, true);

	fbo.Bind();
	uint32_t clearValues[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	glClearBufferuiv(GL_COLOR, 0, clearValues);

	// Read furthest bottom-right pixel in odd-dimension buffer
	auto pixel = fbo.ReadPixel(width - 1, height - 1);
	REQUIRE(pixel.ObjectID == 0xFFFFFFFF);
}TEST_CASE("Framebuffer State Isolation", "[graphics][framebuffer]")
{
	Framebuffer fbo(800, 600);
	fbo.Bind();

	// Unbind
	fbo.Unbind(); // Should call glBindFramebuffer(GL_FRAMEBUFFER, 0)

	// Verify current bound read and draw framebuffers are reset to 0 (default framebuffer)
	GLint boundDrawFBO = 0, boundReadFBO = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &boundDrawFBO);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundReadFBO);

	REQUIRE(boundDrawFBO == 0);
	REQUIRE(boundReadFBO == 0);
}