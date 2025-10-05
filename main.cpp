#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp> // <--- 記得要加上這個！
#include <iostream>
#include <numbers>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "./header/Shader.h"
#include "./header/Object.h"

// Settings
const int INITIAL_SCR_WIDTH = 800;
const int INITIAL_SCR_HEIGHT = 600;
const float fov = 45.0f;
const float AQUARIUM_BOUNDARY=15.0f;
const float AQUARIUM_DEPTH=15.0f;
const double PI = 3.141592653589793;
const float WHRATIO = 800.0f/600.0f;
const float EPISILON = 3e-2f;
// Animation constants
const float TAIL_ANIMATION_SPEED = 5.0f;
const float WAVE_FREQUENCY = 1.5f;

int SCR_WIDTH = INITIAL_SCR_WIDTH;
int SCR_HEIGHT = INITIAL_SCR_HEIGHT;
glm::mat4 baseModel;

// Global objects
Shader* shader = nullptr;
Object* cube = nullptr;
Object* fish1 = nullptr;
Object* fish2 = nullptr;
Object* fish3 = nullptr;

struct Fish {
    glm::vec3 position;
    glm::vec3 direction;
    std::string fishType = "fish1";
    float angle = 0.0f;
    float speed = 5.0f;
    glm::vec3 scale = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f, 0.5f, 0.3f);
};

struct SeaweedSegment {  //This is a node
    glm::vec3 localPos;
    glm::vec3 color;
    float phase;
    glm::vec3 scale;
    SeaweedSegment* next = nullptr;
};
const float segmentHeight =1.5f;

struct Seaweed {  //This is a linked list
    glm::vec3 basePosition;
    SeaweedSegment* rootSegment = nullptr;
    float swayOffset = 0.0f;
};

struct playerFish {
    glm::vec3 position = glm::vec3(0.0f, 5.0f, 0.0f);
    float angle = 0.0f; // Heading direction in radians
    float speed = 2.0f;
    float rotationSpeed = 2.0f;
    bool mouthOpen = false; 
    float tailAnimation = 0.0f;
    // for tooth
    float duration = 1.5f;     
    float elapsed = 0.0f;    
    struct tooth{
        glm::vec3 pos0, pos1;
    }toothUpperLeft, toothUpperRight, toothLowerLeft, toothLowerRight;
   
} playerFish;

// Aquarium elements
std::vector<Seaweed> seaweeds;
std::vector<Fish> schoolFish;

float globalTime = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window, float deltaTime);
void drawModel(std::string type, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color);
void drawPlayerFish(const glm::vec3& position, float angle, float tailPhase,
                    const glm::mat4& view, const glm::mat4& projection,  float deltaTime);
void updateSchoolFish(float deltaTime);
void initializeAquarium();
void cleanup();
void init();

int main() {
    // Initialize random seed for aquarium elements
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // GLFW: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GPU-Accelerated Aquarium", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    // GLAD: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // TODO: Enable depth test, face culling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Initialize Object and Shader
    init();
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f,10.0f,25.0f),glm::vec3(0.0f,8.0f,0.0f),glm::vec3(0.0f,1.0f,0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,1000.0f);
    
    //Initialze acquarium
    initializeAquarium();

    float lastFrame = glfwGetTime();
    //Initialze view,projection matrix
   
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time for the usage of animation
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        globalTime = currentFrame;

        playerFish.tailAnimation += deltaTime * TAIL_ANIMATION_SPEED;

        // Render background
        glClearColor(0.2f, 0.5f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        /*=================== Example of creating model matrix ======================= 
        1. translate
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 1.0f, 0.0f));
        drawModel("cube",model,view,projection, glm::vec3(0.9f, 0.8f, 0.6f));
        
        2. scale
        glm::mat4 model(1.0f);
        model = glm::scale(model, glm::vec3(0.5f, 1.0f, 2.0f)); 
        drawModel("cube",model,view,projection, glm::vec3(0.9f, 0.8f, 0.6f));
        
        3. rotate
        glm::mat4 model(1.0f);
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        drawModel("cube",model,view,projection, glm::vec3(0.9f, 0.8f, 0.6f));
        ==============================================================================*/

        // TODO: Create model, view, and perspective matrix
        
        

        // TODO: Aquarium Base
        
        drawModel("cube",baseModel,view,projection,glm::vec3(0.9f,0.8f,0.6f));
        
        // TODO: Draw seaweeds with hierarchical structure and wave motion
        // Wave motion is sine wave based on global time and segment phase
        // Each segment sways slightly differently for natural effect
        // E.g. Amplitude * sin(keepingChangingX + delayPhase)
        // delayPhase is different for each segment
        // the deeper the segment is, the larger the delayPhase is.
        // so that you can create a forward wave motion.
        const float maxSwing = 0.1f;
        const float omega = 1.5f;
        for(const auto& seaweed : seaweeds){
            glm::mat4 parentModel = glm::translate(glm::mat4(1.0f),seaweed.basePosition);
            SeaweedSegment * currSeg = seaweed.rootSegment;
            while(currSeg!=nullptr){
                float swing = maxSwing*sin(omega*globalTime+currSeg->phase);
                glm::mat4 currentModel = parentModel;
                currentModel = glm::rotate(currentModel,swing,glm::vec3(0.0f,0.0f,1.0f));
                glm::mat4 drawMatrix = glm::translate(currentModel,glm::vec3(0.0f,segmentHeight*0.5f,0.0f));
                drawMatrix = glm::scale(drawMatrix,currSeg->scale);
                drawModel("cube",drawMatrix,view,projection,currSeg->color);
                parentModel = glm::translate(currentModel,currSeg->localPos);  //localPos是高度為segmentHeight的與y軸平行向量
                currSeg = currSeg->next;

            }
        }

        // TODO: Draw school of fish
        // The fish movement logic is implemented.
        // All you need is to set up the position like the example in initAquarium()
        
        for (const auto& fish : schoolFish) {
            
            

            glm::mat4 model(1.0f);
            model = glm::translate(model, fish.position);
            model = glm::rotate(model, fish.angle, glm::vec3(0.0f, 1.0f, 0.0f));   //原本的魚頭是朝向+x方向，因此需要計算初始的tan角來決定魚頭的朝向
            model = glm::scale(model, fish.scale);
            drawModel(fish.fishType, model, view, projection, fish.color);
        }
        // Update aquarium elements
        updateSchoolFish(deltaTime);

        // TODO: Draw Player Fish
        // You can use the provided function drawPlayerFish() or implement your own version.
        // The key idea of hierarchy is to reuse the model matrix to the children.
        // E.g. 
        // glm::mat4 model(1.0f);
        // glm::mat4 bodyModel;
        // model = glm::translate(model, position);
        // ^-- "position": Move the whole body to the desired position.
        // model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));  
        //  ^-- "angle": Rotate the whole body but in the homework case, no need to rotate the fish.
        // bodyModel = glm::scale(model, glm::vec3(5.0f, 3.0f, 2.5f)); // Elongated for shark body
        // drawModel("cube", bodyModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f)); // Dark blue-gray shark color
        // Reuse "model" for the children of the body.
        // glm::mat4 dorsalFinModel;
        // dorsalFinModel = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
        // dorsalFinModel = glm::rotate(dorsalFinModel, glm::radians(-50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // dorsalFinModel = glm::scale(dorsalFinModel, glm::vec3(3.0f, 1.5f, 1.0f));
        // drawModel("cube", dorsalFinModel, view, projection, glm::vec3(0.35f, 0.35f, 0.55f)); // Fin color
        //
        // Notice that to keep the scale of the children is not affected by the body scale,
        // you need to apply the inverse scale to the fin model matrix, 
        // or separate the scale computation from the parent model matrix.
        //
        // For the wave motion of the tail, you can use a sine function based on time,
        // which is provided as playerFish.tailAnimation that would act as tail phase in the drawPlayerFish().
        // To make the tail motion, follow the formula: Amplitude * sin(tailPhase);
        drawPlayerFish(playerFish.position, playerFish.angle, playerFish.tailAnimation,
                        view, projection, deltaTime);

        // TODO: Implement input processing
        processInput(window, deltaTime);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanup();
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void processInput(GLFWwindow* window, float deltaTime) {
    // We use process_input in the display/render loop instead of relying solely on keyCallback
    // because key events (GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT) are not emitted every frame.
    // keyCallback only triggers on discrete key events, but for continuous key behavior (e.g., holding down a key),
    // we need to check key states every frame using glfwGetKey.
    // This ensures smooth, frame-consistent input handling such as movement or rotation.

    // TODO:
    // Controls:
    // - W / S           : Move the fish along the X-axis.
    // - A / D           : Move the fish along the Z-axis.
    // - SPACE / LSHIFT  : Move the fish up / down along the Y-axis.
    //
    // Behavior:
    // - Movement is directly applied to the fish's position on the X, Y, and Z axes.
    // - Vertical movement (Y-axis) is independent of horizontal movement.
    // - The fish can move freely in all three axes but should be clamped within
    //   the aquarium boundaries to stay visible.
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
         
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){

    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        
    }
  
    
    // TODO: Keep fish within aquarium bounds
}
const float mouthDuration = 1.0f;  // 你說大約 1 秒
const float mouthElapsed  = 0.0f;  // 目前進度秒數

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // The action is one of GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE. 
    // Events with GLFW_PRESS and GLFW_RELEASE actions are emitted for every key press.
    // Most keys will also emit events with GLFW_REPEAT actions while a key is held down.
    // https://www.glfw.org/docs/3.3/input_guide.html
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
       playerFish.mouthOpen = !playerFish.mouthOpen;  // 或依你需求改成 true/false

    }
    // TODO: Implement mouth toggle logic


}

void drawModel(std::string type, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color) {
    shader->set_uniform("projection", projection);
    shader->set_uniform("view", view);
    shader->set_uniform("model", model);
    shader->set_uniform("objectColor", color);
    if (type == "fish1") {
        fish1->draw();
    } else if (type == "fish2") {
        fish2->draw();
    } else if (type == "fish3") {
        fish3->draw();
    }else if (type == "cube") {
        cube->draw();
    }
}

void init() {
#if defined(__linux__) || defined(__APPLE__)
    std::string dirShader = "shaders/";
    std::string dirAsset = "asset/";
#else
    std::string dirShader = "shaders\\";
    std::string dirAsset = "asset\\";
#endif

    shader = new Shader((dirShader + "easy.vert").c_str(), (dirShader + "easy.frag").c_str());
   
    cube = new Object(dirAsset + "cube.obj");
    fish1 = new Object(dirAsset + "fish1.obj");
    fish2 = new Object(dirAsset + "fish2.obj");
    fish3 = new Object(dirAsset + "fish3.obj");
}

void cleanup() {
    if (shader) {
        delete shader;
        shader = nullptr;
    }
    
    if (cube) {
        delete cube;
        cube = nullptr;
    }
    
    for (auto& seaweed : seaweeds) {
        SeaweedSegment* current = seaweed.rootSegment;
        while (current != nullptr) {
            SeaweedSegment* next = current->next;
            delete current;
            current = next;
        }
    }
    seaweeds.clear();
    
    schoolFish.clear();
}

void drawPlayerFish(const glm::vec3& position, float angle, float tailPhase,
    const glm::mat4& view, const glm::mat4& projection, float deltaTime) {
    // TODO: Draw body using cube (main body)
    glm::mat4 model(1.0f);
    glm::mat4 bodyModel = glm::translate(model,position);
    bodyModel = glm::rotate(bodyModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 bodyDrawModel = glm::scale(bodyModel, glm::vec3(5.0f, 3.0f, 2.5f)); // Elongated for shark body
    drawModel("cube", bodyDrawModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f)); // Dark blue-gray shark color

    
    
    if (playerFish.mouthOpen) {
        // TODO: head and mouth model matrix adjustment
        glm::mat4 headModel = glm::translate(bodyModel,glm::vec3(3.3f,1.0f,0.0f));
        headModel = glm::rotate(headModel,glm::radians(20.0f),glm::vec3(0.0f,0.0f,1.0f));
        glm::mat4 headDrawModel = glm::scale(headModel,glm::vec3(3.0f,1.75f,2.0f));
        drawModel("cube",headDrawModel, view, projection,glm::vec3(0.4f, 0.4f, 0.6f) );
        //mouth
        glm:: mat4 mouthModel = glm::translate(bodyModel,glm::vec3(3.0f,-1.5f,0.0f));
        mouthModel= glm::rotate(mouthModel,glm::radians(-20.0f),glm::vec3(0.0f,0.0f,1.0f));
        glm::mat4 mouthDrawModel = glm::scale(mouthModel,glm::vec3(2.1f,1.0f,2.0f));
        drawModel("cube",mouthDrawModel, view, projection, glm::vec3(142.0f/255.0f,142.0f/255.0f,142.0f/255.0f)); // Dark blue-gray shark color
        // TODO: Calculate elapse time for tooth animation
        if(playerFish.elapsed<playerFish.duration){
            playerFish.elapsed+=deltaTime;
        }
        else{
            playerFish.mouthOpen =!playerFish.mouthOpen;
            playerFish.elapsed = 0.0f;
        }
        
        
        // TODO: Upper teeth right
        glm::mat4 UpperRightTeethModel = glm::translate(headModel,glm::vec3(1.0f,-0.4f,0.5f));
        // UpperRightTeethModel = glm::scale(UpperRightTeethModel,glm::vec3(0.2f,1.0f,0.4f));
        playerFish.toothUpperRight.pos0 = glm::vec3(UpperRightTeethModel * glm::vec4(0,0,0,1)); //牙齒的原點
        
        glm::mat4 UpperRightTeethModel_plus = glm::translate(headModel,glm::vec3(1.0f,-1.375f,0.5f));  //為了得到pos1的matrix
        playerFish.toothUpperRight.pos1 = glm::vec3(UpperRightTeethModel_plus * glm::vec4(0,0,0,1)); //牙齒最後的座標
        float t  = glm::clamp(playerFish.elapsed / playerFish.duration, 0.0f, playerFish.duration);  //夾在0-1之間
        glm::vec3 UpperRightTeethPosition = glm::mix(playerFish.toothUpperRight.pos0 
        ,playerFish.toothUpperRight.pos1,t);
        glm::mat4 UpperRightTeethDrawMatrix = glm::mat4(glm::mat3(headModel));
        UpperRightTeethDrawMatrix[3] = glm::vec4(UpperRightTeethPosition, 1.0f);
        UpperRightTeethDrawMatrix = glm::scale(UpperRightTeethDrawMatrix,glm::vec3(0.4f,1.0f,0.4f));
        drawModel("cube",UpperRightTeethDrawMatrix, view, projection,glm::vec3(1.0f,1.0f,1.0f) );



        //drawModel("cube",UpperRightTeethModel, view, projection,glm::vec3(1.0f,1.0f,1.0f) ); // Dark blue-gray shark color

        // TODO: Upper teeth left
        glm::mat4 UpperLeftTeethModel = glm::translate(headModel,glm::vec3(0.7f,-0.4f,-0.5f)); //要跟UpperLeftTeethModel_plus的兩個平移量相同
        // UpperLeftTeethModel = glm::scale(UpperLeftTeethModel,glm::vec3(0.2f,1.0f,0.4f));
        playerFish.toothUpperLeft.pos0 = glm::vec3(UpperLeftTeethModel * glm::vec4(0,0,0,1)); //牙齒的原點
        glm::mat4 UpperLeftTeethModel_plus = glm::translate(headModel,glm::vec3(0.7f,-1.375f,-0.5f));  //為了得到pos1的matrix
        playerFish.toothUpperLeft.pos1 = glm::vec3(UpperLeftTeethModel_plus* glm::vec4(0,0,0,1)); //牙齒最後的座標
        
        glm::vec3 UpperLeftTeethPosition = glm::mix(playerFish.toothUpperLeft.pos0 
        ,playerFish.toothUpperLeft.pos1,t);
        glm::mat4 UpperLeftTeethDrawMatrix = glm::mat4(glm::mat3(headModel));
        UpperLeftTeethDrawMatrix[3] = glm::vec4(UpperLeftTeethPosition,1.0f);
        UpperLeftTeethDrawMatrix = glm::scale(UpperLeftTeethDrawMatrix,glm::vec3(0.4f,1.0f,0.4f));
        drawModel("cube",UpperLeftTeethDrawMatrix, view, projection,glm::vec3(1.0f,1.0f,1.0f) );

        // TODO: Lower teeth right 
        // glm::mat4 UpperRightTeethModel = glm::translate(headModel,glm::vec3(1.0f,0.0f,0.5f));
        glm::mat4 LowerRightTeethModel = glm::translate(mouthModel,glm::vec3(0.8f,0.0f,0.5f)); //x的平移量在想一下
        playerFish.toothLowerRight.pos0 = glm::vec3(LowerRightTeethModel* glm::vec4(0,0,0,1));
        glm::mat4 LowerRightTeethModel_plus = glm::translate(mouthModel,glm::vec3(1.0f,1.0f,0.5f));  //為了得到pos1的matrix
        playerFish.toothLowerRight.pos1 = glm::vec3(LowerRightTeethModel_plus* glm::vec4(0,0,0,1));
        glm::vec3 LowerRightTeethPosition = glm::mix(playerFish.toothLowerRight.pos0 
        ,playerFish.toothLowerRight.pos1,t);
         glm::mat4 LowerRightTeethDrawMatrix = glm::mat4(glm::mat3(mouthModel));
        LowerRightTeethDrawMatrix[3] = glm::vec4(LowerRightTeethPosition,1.0f);
        LowerRightTeethDrawMatrix = glm::scale( LowerRightTeethDrawMatrix,glm::vec3(0.4f,1.0f,0.4f));
        drawModel("cube", LowerRightTeethDrawMatrix, view, projection,glm::vec3(1.0f,1.0f,1.0f) );

        // TODO: Lower teeth left
        glm::mat4 LowerLeftTeethModel = glm::translate(mouthModel,glm::vec3(0.8f,0.0f,-0.5f)); //x的平移量在想一下
        playerFish.toothLowerLeft.pos0 = glm::vec3(LowerLeftTeethModel* glm::vec4(0,0,0,1));
        glm::mat4 LowerLeftTeethModel_plus = glm::translate(mouthModel,glm::vec3(1.0f,1.0f,-0.5f));  //為了得到pos1的matrix
        playerFish.toothLowerLeft.pos1 = glm::vec3(LowerLeftTeethModel_plus* glm::vec4(0,0,0,1));
        glm::vec3 LowerLeftTeethPosition = glm::mix(playerFish.toothLowerLeft.pos0 
        ,playerFish.toothLowerLeft.pos1,t);
         glm::mat4 LowerLeftTeethDrawMatrix = glm::mat4(glm::mat3(mouthModel));
        LowerLeftTeethDrawMatrix[3] = glm::vec4(LowerLeftTeethPosition,1.0f);
        LowerLeftTeethDrawMatrix = glm::scale( LowerLeftTeethDrawMatrix,glm::vec3(0.4f,1.0f,0.4f));
        drawModel("cube", LowerLeftTeethDrawMatrix, view, projection,glm::vec3(1.0f,1.0f,1.0f) );
    } 
    else {
         // TODO: Draw head and Mouth using cube with mouth open/close feature
        glm::mat4 headModel = glm::translate(bodyModel,glm::vec3(3.3f,0.5f,0.0f));
        headModel = glm::rotate(headModel,glm::radians(-10.0f),glm::vec3(0.0f,0.0f,1.0f));
        glm::mat4 headDrawModel = glm::scale(headModel,glm::vec3(3.0f,1.75f,2.0f));
        drawModel("cube",headDrawModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f)); // Dark blue-gray shark color
        //mouth
        glm:: mat4 mouthModel = glm::translate(bodyModel,glm::vec3(3.5f,-0.5f,0.0f));
        mouthModel= glm::rotate(mouthModel,glm::radians(10.0f),glm::vec3(0.0f,0.0f,1.0f));
        mouthModel = glm::scale(mouthModel,glm::vec3(2.1f,1.0f,1.0f));
        drawModel("cube",mouthModel, view, projection, glm::vec3(142.0f/255.0f,142.0f/255.0f,142.0f/255.0f)); // Dark blue-gray shark color
        // glm::mat4 UpperLeftTeethModel = glm::translate(headModel,glm::vec3(1.25f,0.0f,-0.5f));
        // UpperLeftTeethModel = glm::scale(UpperLeftTeethModel,glm::vec3(0.2f,1.0f,0.4f));
        // glm::mat4 UpperRightTeethModel = glm::translate(headModel,glm::vec3(1.25f,0.0f,0.5f));
        // UpperRightTeethModel = glm::scale(UpperRightTeethModel,glm::vec3(0.2f,1.0f,0.4f));
        // drawModel("cube",UpperLeftTeethModel, view, projection,glm::vec3(1.0f,1.0f,1.0f) ); // Dark blue-gray shark color
        // drawModel("cube",UpperRightTeethModel, view, projection,glm::vec3(1.0f,1.0f,1.0f)); // Dark blue-gray shark color
   

        // TODO: head and mouth model matrix adjustment
    } 

    // TODO: Draw Eyes
    glm::mat4 eyesModel = glm::translate(bodyModel,glm::vec3(3.3f,0.67f,0.7f));
    eyesModel = glm::scale(eyesModel,glm::vec3(0.5f,0.5f,1.0f));
    drawModel("cube",eyesModel, view, projection, glm::vec3(142.0f/255.0f,142.0f/255.0f,142.0f/255.0f));
    // TODO: Draw Pupils
    glm::mat4 pupilModel = glm::translate(bodyModel,glm::vec3(3.3f,0.67f,0.8f));
    pupilModel = glm::scale(pupilModel,glm::vec3(0.25f,0.25f,1.0f));
    drawModel("cube",pupilModel, view, projection, glm::vec3(0.0f,0.0f,0.0f));
    // TODO: Draw dorsal fin (top fin)
    glm::mat4 dorsalFinModel= glm::translate(bodyModel,glm::vec3(0.75f,1.75f,0.0f));
    dorsalFinModel= glm::rotate(dorsalFinModel,glm::radians(-45.0f),glm::vec3(0.0f,0.0f,1.0f));
    dorsalFinModel = glm::scale(dorsalFinModel,glm::vec3(3.0f,1.0f,1.0f));
    drawModel("cube",dorsalFinModel, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
    // TODO: Draw side fins (pectoral fins)
    //first
    glm::mat4 pectoralFinModel_1 = glm::translate(bodyModel,glm::vec3(0.9f,-1.35f,1.5f));
    pectoralFinModel_1 = glm::rotate(pectoralFinModel_1,glm::radians(30.0f),glm::vec3(1.0f,0.0f,0.0f));
    pectoralFinModel_1 = glm::rotate(pectoralFinModel_1,glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    pectoralFinModel_1 = glm::scale(pectoralFinModel_1,glm::vec3(3.0f,0.3f,1.0f));
    drawModel("cube", pectoralFinModel_1, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
    //second
    glm::mat4 pectoralFinModel_2= glm::translate(bodyModel,glm::vec3(0.9f,-1.35f,-1.5f));
    pectoralFinModel_2 = glm::rotate(pectoralFinModel_2,glm::radians(-30.0f),glm::vec3(1.0f,0.0f,0.0f));
    pectoralFinModel_2 = glm::rotate(pectoralFinModel_2,glm::radians(-45.0f),glm::vec3(0.0f,1.0f,0.0f));
    pectoralFinModel_2 = glm::scale(pectoralFinModel_2,glm::vec3(3.0f,0.3f,1.0f));
    drawModel("cube", pectoralFinModel_2, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
    // TODO: Draw hierarchical animated tail with multiple segments
    //first
    glm::mat4 tailModel_1 = glm::translate(bodyModel,glm::vec3(-3.0f,0.0f,0.0f));
    glm::mat4 tail_drawModel_1 = glm::scale(tailModel_1,glm::vec3(3.0f,1.5f,1.0f));
    drawModel("cube", tail_drawModel_1, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
    //second
    glm::mat4 tailModel_2 = glm::translate(tailModel_1,glm::vec3(-3.0f,0.0f,0.0f));
    glm::mat4 tail_drawModel_2  = glm::scale(tailModel_2,glm::vec3(3.0f,1.25f,1.0f));
    drawModel("cube", tail_drawModel_2, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
    //third
    glm::mat4 tailModel_3 = glm::translate(tailModel_2,glm::vec3(-3.0f,0.0f,0.0f));
    glm::mat4 tail_drawModel_3  = glm::scale(tailModel_3,glm::vec3(3.0f,1.0f,1.0f));
    drawModel("cube", tail_drawModel_3, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
    // TODO: Draw tail fin at the end
    glm::mat4 tailModel_4 = glm::translate(tailModel_3,glm::vec3(-2.0f,0.0f,0.0f));
    glm::mat4 tail_drawModel_4  = glm::scale(tailModel_4,glm::vec3(2.0f,5.0f,1.0f));
    drawModel("cube", tail_drawModel_4, view, projection, glm::vec3(0.4f, 0.4f, 0.6f));
 }

 
void updateSchoolFish(float deltaTime) {
    for (auto& fish : schoolFish) {
        // Move fish in their direction
        // fish.position += fish.direction * fish.speed * deltaTime;
        
        // Bounce off walls
        // The Movement is clamped within aquarium boundaries to prevent
        // fish from escaping the visible scene.
        // atan2 calculates the angle of the fish's direction vector on the XZ plane.
        // To make the fish movement natural.
        if (fish.position.x > (25-fish.position.z)*std::tan(fov*0.5*PI/180.0f)*WHRATIO-3.0f) {
            fish.direction.x *= -1;
            fish.angle = atan2(-fish.direction.z, fish.direction.x);
            fish.position.x = fish.position.x-EPISILON;
        }
        if( fish.position.x < -(25-fish.position.z)*std::tan(fov*0.5*PI/180.0f)*WHRATIO+3.0f ){
            fish.direction.x *= -1;
            fish.angle = atan2(-fish.direction.z, fish.direction.x);
            fish.position.x = fish.position.x+EPISILON;
        }
        if(fish.position.z>AQUARIUM_DEPTH-3.0f ){
            fish.direction.z*=-1;
            fish.angle = atan2(-fish.direction.z, fish.direction.x);
            fish.position.z =  fish.position.z-EPISILON;
        }
        if( fish.position.z<-AQUARIUM_DEPTH+7.0f){
            fish.direction.z*=-1;
            fish.angle = atan2(-fish.direction.z, fish.direction.x);
            fish.position.z =  fish.position.z+EPISILON;
        }
        if(fish.position.y>(25-fish.position.z)*std::tan(fov*0.5*PI/180.0f)){
            fish.direction.y*=-1;
            fish.angle = atan2(-fish.direction.z, fish.direction.x);
            fish.position.y =  fish.position.y-EPISILON;
        }
        if(fish.position.y<-(25-fish.position.z)*std::tan(fov*0.5*PI/180.0f)+3.0f){
            fish.direction.y*=-1;
            fish.angle = atan2(-fish.direction.z, fish.direction.x);
            fish.position.y =  fish.position.y+EPISILON;
        }
    }
}


void initializeAquarium() {
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f,10.0f,25.0f),glm::vec3(0.0f,8.0f,0.0f),glm::vec3(0.0f,1.0f,0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)SCR_WIDTH/(float)SCR_HEIGHT,0.1f,1000.0f);
    srand(static_cast<unsigned int>(time(nullptr)));
    baseModel = glm::mat4(1.0f);
    baseModel = glm::scale(baseModel,glm::vec3(70.0f,1.0f,40.0f));

    //initialize seaweeds
    std::vector<glm::vec3> seaweedsPositions ={
        glm::vec3(7.0f,0.0f,0.0f),
        glm::vec3(-7.0f,0.0f,-10.0f),
        glm::vec3(-7.0f,0.0f,5.0f)
    };
    const int numOfSegment = 7;
   
    const float delayPerSegment = 0.3f;
    for(const auto& pos:seaweedsPositions){
        float maxSwing = 0.25f;
        float omega = 1.8f;
        Seaweed seaweed;
        seaweed.basePosition = pos;
        SeaweedSegment * prev = nullptr;
        for(int i =0 ;i<numOfSegment;++i){
            SeaweedSegment * newSeg = new SeaweedSegment();
            newSeg->localPos = glm::vec3(0.0f,segmentHeight,0.0f);
            newSeg->color = glm::vec3(0.0f,0.5f,0.0f);
            newSeg->phase = i*delayPerSegment;
            newSeg->scale = glm::vec3(1.0f,segmentHeight,1.0f);
            newSeg->next = nullptr;
            if(prev == nullptr)
                seaweed.rootSegment = newSeg;
            else
                prev->next = newSeg;   
            prev = newSeg;
            seaweeds.push_back(seaweed);


        }
    }
    const std::vector<std::pair<std::string, glm::vec3>> initialFishData = {
            {"fish1", glm::vec3(0.0f, 15.0f, 0.0f)},
            {"fish2", glm::vec3(7.0f, 3.0f, 0.0f)},
            {"fish3", glm::vec3(-3.0f, 7.0f, -7.0f)}
        };
        std::vector<glm::vec3> colorVector={
            glm::vec3(1.0f,0.5f,0.0f),
            glm::vec3(0.0f,0.5f,1.0f),
            glm::vec3(0.0f,0.73f,0.0f)
        };
        int colorIndex = 0;
        schoolFish.clear();
        for(const auto& data : initialFishData){
            Fish newFish;
            newFish.fishType = data.first;
            newFish.position = data.second;
            const float randomAngle = static_cast<float> (rand())/(RAND_MAX)*2*3.14159f;
            newFish.direction = glm::vec3(cos(randomAngle),0.0f,sin(randomAngle)); 
            newFish.color = colorVector[colorIndex];
            colorIndex++;
            newFish.angle = atan2(-(newFish.direction.z),newFish.direction.x);
            schoolFish.push_back(newFish);
        }


    // You can init the aquarium elements here
    // e.g.
    // playerFish.toothUpperRight.pos0 = glm::vec3(1.0f, -0.3f, 0.5f);
    // playerFish.toothUpperRight.pos1 = glm::vec3(1.0f, -1.5f, 0.5f);
    //
    // schoolFish.clear();
    // const std::map<std::string, glm::vec3> FishPositions {
    //     {"fish1", glm::vec3(0.0f, 15.0f, 0.0f)},
    // };
    // for (const auto& [name, pos] : FishPositions) {
    //     Fish fish;
    //     fish.position = pos;
    //     fish.direction = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
    //     fish.fishType = name;
    //     fish.color = glm::vec3(static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX);
    //     schoolFish.push_back(fish);
    // }
}
