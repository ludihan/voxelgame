#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/id.h"
#include "box3d/math_functions.h"
#include "box3d/types.h"
#include "raylib.h"

#include "raymath.h"
#include "rlgl.h"
#include <stdio.h>

#define i_key b3BodyId
#include "stc/vec.h"

#define i_key b3ShapeId
#include "stc/vec.h"

// Movement constants
#define GRAVITY 32.0f
#define MAX_SPEED 20.0f
#define CROUCH_SPEED 5.0f
#define JUMP_FORCE 12.0f
#define MAX_ACCEL 150.0f
// Grounded drag
#define FRICTION 0.86f
// Increasing air drag, increases strafing speed
#define AIR_DRAG 0.98f
// Responsiveness for turning movement direction to looked direction
#define CONTROL 15.0f
#define CROUCH_HEIGHT 0.0f
#define STAND_HEIGHT 1.0f
#define BOTTOM_HEIGHT 0.5f

#define NORMALIZE_INPUT 0

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

typedef struct {
    b3BodyId *bodies;
} World;

static Vector2 sensitivity = {0.001f, 0.001f};
static Body player = {0};
static Vector2 lookRotation = {0};
static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;
static Vector2 lean = {0};

// Update body considering current world state
void update_body(
    Body *body,
    float rot,
    char side,
    char forward,
    bool jumpPressed,
    bool crouchHold
) {
    Vector2 input = (Vector2){(float)side, (float)-forward};

#if defined(NORMALIZE_INPUT)
    // Slow down diagonal movement
    if ((side != 0) && (forward != 0))
        input = Vector2Normalize(input);
#endif

    float delta = GetFrameTime();

    if (!body->isGrounded)
        body->velocity.y -= GRAVITY * delta;

    if (body->isGrounded && jumpPressed) {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;

        // Sound can be played at this moment
        // SetSoundPitch(fxJump, 1.0f + (GetRandomValue(-100, 100)*0.001));
        // PlaySound(fxJump);
    }

    Vector3 front = (Vector3){sinf(rot), 0.f, cosf(rot)};
    Vector3 right = (Vector3){cosf(-rot), 0.f, sinf(-rot)};

    Vector3 desiredDir = (Vector3){
        input.x * right.x + input.y * front.x,
        0.0f,
        input.x * right.z + input.y * front.z,
    };
    body->dir = Vector3Lerp(body->dir, desiredDir, CONTROL * delta);

    float decel = (body->isGrounded ? FRICTION : AIR_DRAG);
    Vector3 hvel =
        (Vector3){body->velocity.x * decel, 0.0f, body->velocity.z * decel};

    float hvelLength = Vector3Length(hvel); // Magnitude
    if (hvelLength < (MAX_SPEED * 0.01f))
        hvel = (Vector3){0};

    // This is what creates strafing
    float speed = Vector3DotProduct(hvel, body->dir);

    // Whenever the amount of acceleration to add is clamped by the maximum
    // acceleration constant, a Player can make the speed faster by bringing the
    // direction closer to horizontal velocity angle More info here:
    // https://youtu.be/v3zT3Z5apaM?t=165
    float maxSpeed = (crouchHold ? CROUCH_SPEED : MAX_SPEED);
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL * delta);
    hvel.x += body->dir.x * accel;
    hvel.z += body->dir.z * accel;

    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    body->position.x += body->velocity.x * delta;
    body->position.y += body->velocity.y * delta;
    body->position.z += body->velocity.z * delta;

    // Fancy collision system against the floor
    if (body->position.y <= 0.0f) {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true; // Enable jumping
    }
}

// Update camera for FPS behaviour
static void update_camera_fps(Camera *camera) {
    const Vector3 up = (Vector3){0.0f, 1.0f, 0.0f};
    const Vector3 targetOffset = (Vector3){0.0f, 0.0f, -1.0f};

    // Left and right
    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    // Clamp view up
    float maxAngleUp = Vector3Angle(up, yaw);
    maxAngleUp -= 0.001f; // Avoid numerical errors
    if (-(lookRotation.y) > maxAngleUp) {
        lookRotation.y = -maxAngleUp;
    }

    // Clamp view down
    float maxAngleDown = Vector3Angle(Vector3Negate(up), yaw);
    maxAngleDown *= -1.0f;  // Downwards angle is negative
    maxAngleDown += 0.001f; // Avoid numerical errors
    if (-(lookRotation.y) < maxAngleDown) {
        lookRotation.y = -maxAngleDown;
    }

    // Up and down
    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    // Rotate view vector around right axis
    float pitchAngle = -lookRotation.y - lean.y;
    pitchAngle = Clamp(
        pitchAngle, -PI / 2 + 0.0001f, PI / 2 - 0.0001f
    ); // Clamp angle so it doesn't go past straight up or straight down
    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    // Head animation
    // Rotate up direction around forward axis
    float headSin = sinf(headTimer * PI);
    float headCos = cosf(headTimer * PI);
    const float stepRotation = 0.01f;
    camera->up =
        Vector3RotateByAxisAngle(up, pitch, headSin * stepRotation + lean.x);

    // Camera BOB
    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    Vector3 bobbing = Vector3Scale(right, headSin * bobSide);
    bobbing.y = fabsf(headCos * bobUp);

    camera->position =
        Vector3Add(camera->position, Vector3Scale(bobbing, walkLerp));
    camera->target = Vector3Add(camera->position, pitch);
}

static b3ShapeId setup_level(b3WorldId worldId) {
    b3BodyDef levelDef = b3DefaultBodyDef();
    levelDef.type = b3_staticBody;
    levelDef.position = (b3Vec3){0.0f, 0.0f, 0.0f};
    b3BodyId levelId = b3CreateBody(worldId, &levelDef);
    b3BoxHull planeBox = b3MakeBoxHull(25.0f, 0.1f, 25.0f);
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.baseMaterial.friction = 0.3f;
    return b3CreateHullShape(levelId, &shapeDef, &planeBox.base);
}

static b3ShapeId create_cube(b3WorldId worldId) {
    b3BodyDef cubeBody = b3DefaultBodyDef();
    cubeBody.type = b3_dynamicBody;
    cubeBody.position = (b3Vec3){
        0.0f + (float)GetRandomValue(-20, 20),
        20.0f,
        0.0f + (float)GetRandomValue(-20, 20)
    };
    b3BodyId cubeId = b3CreateBody(worldId, &cubeBody);
    b3BoxHull dynamicBox = b3MakeCubeHull(1.0f);
    b3ShapeDef cubeShape = b3DefaultShapeDef();
    cubeShape.density = 1.0f;
    cubeShape.baseMaterial.friction = 0.3f;
    return b3CreateHullShape(cubeId, &cubeShape, &dynamicBox.base);
}

static vec_b3ShapeId create_walls(b3WorldId worldId) {
    vec_b3ShapeId wall_shapes = {0};
    for (size_t i = 0; i < 4; i++) {
        b3BodyDef wallBody = b3DefaultBodyDef();
        switch (i) {
        case 0:
        case 1:
            wallBody.position = (b3Vec3){
                0.0f,
                5.0f,
                (i == 0 ? 1 : -1) * 20.0f,
            };
            break;
        case 2:
        case 3:
            wallBody.position = (b3Vec3){(i == 2 ? 1 : -1) * 20.0f, 5.0f, 0.0f};
            break;
        }
        wallBody.type = b3_kinematicBody;
        b3BodyId wallId = b3CreateBody(worldId, &wallBody);
        b3BoxHull staticWall = b3MakeBoxHull(15.0f, 5.0f, 2.0f);
        b3ShapeDef wallShape = b3DefaultShapeDef();
        wallShape.density = 1.0f;
        wallShape.baseMaterial.friction = 0.3f;
        vec_b3ShapeId_push(
            &wall_shapes,
            b3CreateHullShape(wallId, &wallShape, &staticWall.base)
        );
    }
    return wall_shapes;
}

int main(void) {
    bool free_camera = true;
    const int screenWidth = 1024;
    const int screenHeight = 768;

    vec_b3BodyId cube_bodies = {0};
    vec_b3BodyId wall_bodies = {0};

    InitWindow(
        screenWidth, screenHeight, "raylib [core] example - 3d camera fps"
    );

    b3WorldDef worldDef = b3DefaultWorldDef();
    worldDef.gravity = (b3Vec3){0.0f, -10.0f, 0.0f};
    b3WorldId worldId = b3CreateWorld(&worldDef);
    setup_level(worldId);
    vec_b3ShapeId walls = create_walls(worldId);
    c_foreach(shapeId, vec_b3ShapeId, walls) {
        vec_b3BodyId_push(&wall_bodies, b3Shape_GetBody(*shapeId.ref));
    }

    Camera camera = {0};
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.position = (Vector3){
        player.position.x,
        player.position.y + (BOTTOM_HEIGHT + headLerp),
        player.position.z,
    };

    update_camera_fps(&camera);

    DisableCursor();

    SetTargetFPS(60);
    // Mesh cubeMesh = GenMeshCube(2.0f, 2.0f, 2.0f);
    Model cubeModel = LoadModel("res/raylib_cube.glb");
    cubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;

    bool veloc_appli = false;
    int i = 0;
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE))
            free_camera = false;
        if (IsKeyPressed(KEY_TWO))
            free_camera = true;
        if (IsKeyPressed(KEY_F)) {
            ToggleFullscreen();
        }
        if (IsKeyPressed(KEY_C)) {
            for (size_t i = 0; i < 10; i++) {
                b3ShapeId shapeId = create_cube(worldId);
                b3BodyId bodyId = b3Shape_GetBody(shapeId);
                vec_b3BodyId_push(&cube_bodies, bodyId);
            }
        }

        if (!free_camera) {
            Vector2 mouseDelta = GetMouseDelta();
            lookRotation.x -= mouseDelta.x * sensitivity.x;
            lookRotation.y += mouseDelta.y * sensitivity.y;

            char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
            char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
            bool crouching = IsKeyDown(KEY_LEFT_CONTROL);
            update_body(
                &player,
                lookRotation.x,
                sideway,
                forward,
                IsKeyPressed(KEY_SPACE),
                crouching
            );
            float delta = GetFrameTime();
            headLerp = Lerp(
                headLerp,
                (crouching ? CROUCH_HEIGHT : STAND_HEIGHT),
                20.0f * delta
            );
            camera.position = (Vector3){
                player.position.x,
                player.position.y + (BOTTOM_HEIGHT + headLerp),
                player.position.z,
            };

            if (player.isGrounded && ((forward != 0) || (sideway != 0))) {
                headTimer += delta * 3.0f;
                walkLerp = Lerp(walkLerp, 1.0f, 10.0f * delta);
                camera.fovy = Lerp(camera.fovy, 55.0f, 5.0f * delta);
            } else {
                walkLerp = Lerp(walkLerp, 0.0f, 10.0f * delta);
                camera.fovy = Lerp(camera.fovy, 60.0f, 5.0f * delta);
            }

            lean.x = Lerp(lean.x, sideway * 0.02f, 10.0f * delta);
            lean.y = Lerp(lean.y, forward * 0.015f, 10.0f * delta);

            update_camera_fps(&camera);

        } else {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        c_foreach(bodyId, vec_b3BodyId, cube_bodies) {
            b3Vec3 position = b3Body_GetPosition(*bodyId.ref);
            b3Quat rotation = b3Body_GetRotation(*bodyId.ref);
            Matrix matRotation = QuaternionToMatrix((Quaternion){
                rotation.v.x, rotation.v.y, rotation.v.z, rotation.s
            });
            Matrix matTranslation =
                MatrixTranslate(position.x, position.y, position.z);
            Matrix matModel = MatrixMultiply(matRotation, matTranslation);
            cubeModel.transform = matModel;

            DrawModel(cubeModel, (Vector3){0, 0, 0}, 1.0f, WHITE);
        }

        c_foreach(bodyId, vec_b3BodyId, wall_bodies) {
            b3ShapeId a[1] = {0};
            b3Body_GetShapes(*bodyId.ref, (b3ShapeId *)&a, 1);
            const b3HullData *data = b3Shape_GetHull(a[0]);
            if (!veloc_appli) {
                b3Body_SetAngularVelocity(
                    *bodyId.ref, (b3Vec3){0.0f, 2.0f, 0.0f}
                );
                i++;
                if (i == 4)
                    veloc_appli = true;
            }
            b3AABB aabb = data->aabb;
            b3Vec3 position = b3Body_GetPosition(*bodyId.ref);
            b3Quat rotation = b3Body_GetRotation(*bodyId.ref);

            Matrix matRotation = QuaternionToMatrix((Quaternion){
                rotation.v.x, rotation.v.y, rotation.v.z, rotation.s
            });
            Matrix matTranslation =
                MatrixTranslate(position.x, position.y, position.z);
            Matrix matModel = MatrixMultiply(matRotation, matTranslation);
            cubeModel.transform = matModel;
            float angle;
            b3Vec3 axis = b3GetAxisAngle(&angle, rotation);

            rlPushMatrix();

            rlTranslatef(position.x, position.y, position.z);

            rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);
            printf("%f %f %f\n", position.x, position.y, position.z);

            DrawCube(
                (Vector3){0.0f, 0.0f, 0.0f},
                fabsf(aabb.lowerBound.x - aabb.upperBound.x),
                fabsf(aabb.lowerBound.y - aabb.upperBound.y),
                fabsf(aabb.lowerBound.z - aabb.upperBound.z),
                RED
            );

            rlPopMatrix();
        }

        DrawGrid(100, 1.0f);
        EndMode3D();

        EndDrawing();
        float timeStep = GetFrameTime();
        int subStepCount = 4;
        b3World_Step(worldId, timeStep, subStepCount);
    }

    CloseWindow();

    b3DestroyWorld(worldId);
    vec_b3BodyId_drop(&cube_bodies);
    vec_b3BodyId_drop(&wall_bodies);
    return 0;
}
