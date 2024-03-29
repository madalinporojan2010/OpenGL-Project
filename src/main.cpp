#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp" //core glm functionality
#include "glm/gtc/matrix_transform.hpp" //glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp" //glm extension for computing inverse matrices
#include "glm/gtc/type_ptr.hpp" //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// skybox
gps::SkyBox mySkyBox;
std::vector<const GLchar*> faces;

// Scene proprieties
const float fov = 1000.0f;

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;

const GLfloat near_plane = 0.1f, far_plane = 30.0f;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 lightYmovement;
glm::mat4 mainLightSpaceTrMatrix;   
glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -5.0f, 5.0f, near_plane, far_plane);
glm::mat4 lightView;

// light parameters

struct LightStruct {
    glm::vec3 lightDir;
    glm::vec3 lightColor;
    GLint lightDirLoc;
    GLint lightColorLoc;
    glm::mat4 lightRotation;
    GLfloat lightBrightness;
} mainLight, secondaryLight;


// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.05f;

GLboolean pressedKeys[1024];
//auto camera
float cameraAngle = 0.0f;
float cameraRadius = 10.0f;
bool beginCameraAnimation = false;

// models
gps::Model3D screenQuad;
gps::Model3D ground;

gps::Model3D windows;

// animated models
gps::Model3D frontDoor;
float frontDoorRotationAngle = 0.0f;

bool beginFrontDoorAnimation = false;

// light models
gps::Model3D lightCube;
GLfloat lightAngle = 0.0f;
float angleY = 0.0f;
float Ypos = 1.0f;
//shadows
GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap = false;

// shaders
gps::Shader myCustomShader;
//gps::Shader reflectionShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

gps::Shader skyBoxShader;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

float lastX, lastY;
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float Xoffset = xpos - lastX, Yoffset = lastY - ypos;

    const float sentitivity = 0.1f;
    Xoffset *= sentitivity;
    Yoffset *= sentitivity;
    lastX = xpos;
    lastY = ypos;

    yaw += Xoffset;
    pitch += Yoffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }
    if (pitch < -89.0f) {
        pitch = -89.0f;
    }
    // converted x sin yaw to cos (switched with z) and added - to z cos
    myCamera.rotate(pitch, yaw);

}


void processMovement() {
    if (pressedKeys[GLFW_KEY_Q]) {
        angleY -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angleY += 1.0f;
    }

    if (pressedKeys[GLFW_KEY_I]) {
        Ypos -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_K]) {
        Ypos += 1.0f;
    }


    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    }

    //light

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
    }

    if (pressedKeys[GLFW_KEY_I]) {
        if (mainLight.lightBrightness > 0.1f)
            mainLight.lightBrightness -= 0.01f;
    }

    if (pressedKeys[GLFW_KEY_P]) {
        if (mainLight.lightBrightness < 0.9f)
            mainLight.lightBrightness += 0.01f;
    }

    if (pressedKeys[GLFW_KEY_8]) {
        if (secondaryLight.lightBrightness > 0.1f)
            secondaryLight.lightBrightness -= 0.01f;
    }

    if (pressedKeys[GLFW_KEY_0]) {
        if (secondaryLight.lightBrightness < 0.9f)
            secondaryLight.lightBrightness += 0.01f;
    }

    if (pressedKeys[GLFW_KEY_N]) {
        if (frontDoorRotationAngle < 90.0f)
            frontDoorRotationAngle += 1.0f;
    }

    if (pressedKeys[GLFW_KEY_B]) {
        if (frontDoorRotationAngle > 0.99f)
            frontDoorRotationAngle -= 1.0f;
    }

    //animations

    if (pressedKeys[GLFW_KEY_Z]) {
        // reset door animation
        beginFrontDoorAnimation = false;
        frontDoorRotationAngle = 0.0f;
    }
    if (pressedKeys[GLFW_KEY_C]) {
        // begin door animation
        beginFrontDoorAnimation = true;
    }

    // type of render

    if (pressedKeys[GLFW_KEY_R]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // solid mode
    }
    if (pressedKeys[GLFW_KEY_T]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
    }
    if (pressedKeys[GLFW_KEY_Y]) {
        glShadeModel(GL_FLAT); // polygonal shade mode
    }
    if (pressedKeys[GLFW_KEY_F]) {
        glShadeModel(GL_SMOOTH); // smooth shade mode
    }

    // camera animation
    if (pressedKeys[GLFW_KEY_5]) {
        beginCameraAnimation = false;
        cameraAngle = 0.0f;
    }
    if (pressedKeys[GLFW_KEY_6]) {
        beginCameraAnimation = true;
    }
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    ground.LoadModel("models/terrain/landscape.obj");
    lightCube.LoadModel("models/cube/cube.obj");
    screenQuad.LoadModel("models/quad/quad.obj");
    frontDoor.LoadModel("models/doors/front_Door.obj");
    windows.LoadModel("models/windows/balcony-windows.obj");
    mySkyBox.Load(faces);
}

void initShaders() {
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    myCustomShader.useShaderProgram();
    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    lightShader.useShaderProgram();
    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    screenQuadShader.useShaderProgram();
    depthMapShader.loadShader("shaders/shadowMap.vert", "shaders/shadowMap.frag");
    depthMapShader.useShaderProgram();
    skyBoxShader.loadShader("shaders/skyBoxShader.vert", "shaders/skyBoxShader.frag");
    skyBoxShader.useShaderProgram();
    //reflectionShader.loadShader("shaders/reflectionShader.vert", "shaders/reflectionShader.frag");
    //reflectionShader.useShaderProgram();

}

void initUniforms(gps::Shader shader) {
    shader.useShaderProgram();

    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(shader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, fov);
    projectionLoc = glGetUniformLocation(shader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    mainLight.lightDir = glm::vec3(15.438160f, 12.868689f, -7.212670f);
    mainLight.lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    mainLight.lightDirLoc = glGetUniformLocation(shader.shaderProgram, "mainLightDir");
    glUniform3fv(mainLight.lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * mainLight.lightRotation)) * mainLight.lightDir));
      
    //set the light direction (direction towards the light)
    secondaryLight.lightDir = glm::vec3(-10.688848f, 3.203635f, 0.789529f);
    secondaryLight.lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    secondaryLight.lightDirLoc = glGetUniformLocation(shader.shaderProgram, "secondaryLightDir");
    glUniform3fv(secondaryLight.lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * secondaryLight.lightRotation)) * secondaryLight.lightDir));

    //set light color
    mainLight.lightColor = glm::vec3(1.0f * mainLight.lightBrightness, 1.0f * mainLight.lightBrightness, 1.0f * mainLight.lightBrightness); //white light
    mainLight.lightColorLoc = glGetUniformLocation(shader.shaderProgram, "mainLightColor");
    glUniform3fv(mainLight.lightColorLoc, 1, glm::value_ptr(mainLight.lightColor));

    //set light color
    secondaryLight.lightColor = glm::vec3(0.0f * secondaryLight.lightBrightness, 0.0f * secondaryLight.lightBrightness, 1.0f * secondaryLight.lightBrightness); //white light
    secondaryLight.lightColorLoc = glGetUniformLocation(shader.shaderProgram, "secondaryLightColor");
    glUniform3fv(secondaryLight.lightColorLoc, 1, glm::value_ptr(secondaryLight.lightColor));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

void initFBO() {
    //Generate fbo id
    glGenFramebuffers(1, &shadowMapFBO);

    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initSkyBox() {
    faces.push_back("skybox/frozen_rt.tga");
    faces.push_back("skybox/frozen_lf.tga");
    faces.push_back("skybox/frozen_up.tga");
    faces.push_back("skybox/frozen_dn.tga");
    faces.push_back("skybox/frozen_bk.tga");
    faces.push_back("skybox/frozen_ft.tga");
    
}

glm::mat4 computeMainLightSpaceTrMatrix() {
    lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(mainLight.lightRotation)) * mainLight.lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

void renderLandScape(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    glm::mat4 landScapeModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    landScapeModel = glm::scale(landScapeModel, glm::vec3(0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(landScapeModel));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * landScapeModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    ground.Draw(shader);
}

void renderWindows(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    glm::mat4 windowsModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    windowsModel = glm::scale(windowsModel, glm::vec3(0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(windowsModel));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * windowsModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    windows.Draw(shader);
}


void renderFrontDoor(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    glm::mat4 frontDoorModel = glm::mat4(1.0f);
    frontDoorModel = glm::translate(frontDoorModel, glm::vec3(-10.555405f, 2.280203f, 0.319486f));
    frontDoorModel = glm::scale(frontDoorModel, glm::vec3(0.5f));


    if (beginFrontDoorAnimation && frontDoorRotationAngle < 90.0f) {
        frontDoorRotationAngle += 0.5f ;
    }
    frontDoorModel = glm::rotate(frontDoorModel, glm::radians(frontDoorRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    


    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(frontDoorModel));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * frontDoorModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    frontDoor.Draw(shader);
}


glm::vec3 P0(-21.979586f, 4.031127f, -4.629179f);
glm::vec3 P1(-13.868981f, 3.258924f, -1.217223f);
glm::vec3 P2(-11.700786f, 2.799728f, 0.451809f);
glm::vec3 P3(-0.127710f, 4.919400f, 9.089221f);
glm::vec3 P4(-1.076682f, 6.379920f, 13.895396f);
glm::vec3 P5(15.770725f, 6.901012f, 4.650489f);
glm::vec3 P6(18.986069f, 8.740207f, -18.393059f);
glm::vec3 P7(7.216824f, 6.534041f, -23.205952f);
glm::vec3 P8(-6.591795f, 6.500103f, -15.904457f);
glm::vec3 P9(-22.149868f, 5.723304f, -15.828244f);
glm::vec3 P10(-35.216900f, 5.302893f, -12.221725f);
glm::vec3 P11(-34.724472f, 5.880974f, 9.490473f);
glm::vec3 P12(-21.979586f, 4.031127f, -4.629179f);

std::vector<glm::vec3> bezierPositionPoints;

float t = 0.0f;
float speed = 0.0006f;

void initControlPoints() {
    bezierPositionPoints.push_back(P0);
    bezierPositionPoints.push_back(P1);
    bezierPositionPoints.push_back(P2);
    bezierPositionPoints.push_back(P3);
    bezierPositionPoints.push_back(P4);
    bezierPositionPoints.push_back(P5);
    bezierPositionPoints.push_back(P6);
    bezierPositionPoints.push_back(P7);
    bezierPositionPoints.push_back(P8);
    bezierPositionPoints.push_back(P9);
    bezierPositionPoints.push_back(P10);
    bezierPositionPoints.push_back(P11);
    bezierPositionPoints.push_back(P12);
}

float binomialCoefficient(int n, int k) {
    float res = 1;
    if (k > n - k)
        k = n - k;
    for (int i = 0; i < k; ++i) {
        res *= (n - i);
        res /= (i + 1);
    }
    return res;
}

glm::vec3 calculateBezierCurve(std::vector<glm::vec3> controlPoints, float t) {
    int n = controlPoints.size() - 1;
    glm::vec3 position(0);
    for (int i = 0; i <= n; i++) {
        float binom = binomialCoefficient(n, i);
        float bernstein = binom * pow(1 - t, n - i) * pow(t, i);
        position += bernstein * controlPoints[i];
    }
    return position;
}

void renderScene() {

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "mainLightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeMainLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    renderLandScape(depthMapShader, true);
    renderWindows(depthMapShader, true);
    renderFrontDoor(depthMapShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT);


    if (showDepthMap) {
        screenQuadShader.useShaderProgram();


        //bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);




        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);




    }
    else {

        // final scene rendering pass (with shadows)

        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        

        myCustomShader.useShaderProgram();

        if (!beginCameraAnimation) {
            view = myCamera.getViewMatrix();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }
        else {
            // Update angle
            t += speed;
            t = fmod(t, 1.0f);

            // Calculate camera's position
            myCamera.cameraPosition = calculateBezierCurve(bezierPositionPoints, t);

            view = glm::lookAt(myCamera.cameraPosition, glm::vec3(-5.280864f, 3.254189f, 2.045167f), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }

        mainLight.lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(mainLight.lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * mainLight.lightRotation)) * mainLight.lightDir));

        mainLight.lightColor = glm::vec3(1.0f * mainLight.lightBrightness, 1.0f * mainLight.lightBrightness, 1.0f * mainLight.lightBrightness); //white light
        glUniform3fv(mainLight.lightColorLoc, 1, glm::value_ptr(mainLight.lightColor));

        secondaryLight.lightColor = glm::vec3(0.0f * secondaryLight.lightBrightness, 0.0f * secondaryLight.lightBrightness, 1.0f * secondaryLight.lightBrightness); //white light
        glUniform3fv(secondaryLight.lightColorLoc, 1, glm::value_ptr(secondaryLight.lightColor));

        //bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

        glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "mainLightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeMainLightSpaceTrMatrix()));
        
        renderLandScape(myCustomShader, false);

        glEnable(GL_BLEND); // transparenta
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // transparenta
        renderWindows(myCustomShader, false);
        glDisable(GL_BLEND); // transparenta

        renderFrontDoor(myCustomShader, false);

        //draw a white cube around the light

        lightShader.useShaderProgram();

        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        model = mainLight.lightRotation;
        model = glm::translate(model, 1.0f * mainLight.lightDir);
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        lightCube.Draw(lightShader);


    }
    mySkyBox.Draw(skyBoxShader, view, projection);
}

void cleanup() {
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    myWindow.Delete();
    //cleanup code for your own data
}

void initLightProps() {
    mainLight.lightBrightness = 1.0f;
    secondaryLight.lightBrightness = 1.0f;
}


int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initControlPoints();
    initOpenGLState();
    initLightProps();
    initSkyBox();
	initModels();
	initShaders();
	initUniforms(myCustomShader);
	//initUniforms(reflectionShader);
    setWindowCallbacks();
    initFBO();

	glCheckError();
	// application loop
    
    // FPS
    double lastFrameTime = glfwGetTime();
    double dSum = 0.0f;
    int step = 0;

    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        // FPS
        step++;

        processMovement();
        renderScene();
		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

        glCheckError();



        double currentFrameTime = glfwGetTime();
        double dFrameTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        dSum += dFrameTime;
        if (step >= 20) {
            //std::cout << step / dSum << std::endl;
            step = 0;
            dSum = 0.0f;
        }
	}

	cleanup();

    return EXIT_SUCCESS;
}
