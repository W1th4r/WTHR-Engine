function player_controller_onInit(entity)
    print("Player controller initialized for entity: " .. entity)
    local transform = getTransform(entity)
    if transform then
        transform.position.x = 0.0
        transform.position.y = 0.0
        transform.position.z = 0.0
    end
end

-- Define movement speed (units per second)
local speed = 0.5

function player_controller_onUpdate(entity, dt)
    local transform = getTransform(entity)

    if transform then
        -- Initialize movement vectors for this frame
        local moveX = 0.0
        local moveZ = 0.0 -- Using Z for forward/backward if 3D, change to Y if 2D

        -- Check inputs (assuming keys are passed as strings or character codes)
        if isKeyPressed("W") then
            moveZ = moveZ + 1.0
        end
        if isKeyPressed("S") then
            moveZ = moveZ - 1.0
        end
        if isKeyPressed("A") then
            moveX = moveX - 1.0
        end
        if isKeyPressed("D") then
            moveX = moveX + 1.0
        end

        -- Update the position based on speed and delta time (dt)
        transform.position.x = transform.position.x + (moveX * speed * dt)
        transform.position.z = transform.position.z + (moveZ * speed * dt) 
        -- Note: If your game is 2D, change 'transform.position.z' to 'transform.position.y' above!

        -- Optional: Print debugging position occasionally
        -- print("Player Position: X=" .. transform.position.x .. ", Z=" .. transform.position.z)
    end
end

function player_controller_onDestroy(entity)
    print("Player controller destroyed for entity: " .. entity)
end