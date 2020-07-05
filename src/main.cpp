// openxr example


#define XR_USE_GRAPHICS_API_OPENGL
#if defined(WIN32)
#define XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#else
#define XR_USE_PLATFORM_XLIB
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <stdlib.h> //rand()

static bool quitting = false;
static float r = 0.0f;
static SDL_Window *window = NULL;
static SDL_GLContext gl_context;
static SDL_Renderer *renderer = NULL;


void render()
{
    SDL_GL_MakeCurrent(window, gl_context);
    r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    glClearColor(r, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(window);
}


int SDLCALL watch(void *userdata, SDL_Event* event)
{
    if (event->type == SDL_APP_WILLENTERBACKGROUND)
    {
        quitting = true;
    }

    return 1;
}

static bool CheckResult(XrInstance instance, XrResult result, const char* str)
{
    if (XR_SUCCEEDED(result))
    {
        return true;
    }

    if (instance)
    {
        char resultString[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(instance, result, resultString);
        printf("%s [%s]\n", str, resultString);
    }
    else
    {
        printf("%s\n", str);
    }
    return false;
}

bool GLSupport()
{
    XrResult result;
    uint32_t extensionCount = 0;
    result = xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);
    if (!CheckResult(NULL, result, "xrEnumerateInstanceExtensionProperties failed"))
    {
        return false;
    }

    std::vector<XrExtensionProperties> extensionProperties(extensionCount);
    for (uint32_t i = 0; i < extensionCount; i++) {
        extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
        extensionProperties[i].next = NULL;
    }
    result = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, extensionProperties.data());
    if (!CheckResult(NULL, result, "xrEnumerateInstanceExtensionProperties failed"))
    {
        return false;
    }

    bool printExtensions = false;
    if (printExtensions)
    {
        printf("extensions:\n");
    }

    bool glSupported = false;
    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        if (!strcmp(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, extensionProperties[i].extensionName))
        {
            glSupported = true;
        }

        if (printExtensions)
        {
            printf("extensions: %s\n", extensionProperties[i].extensionName);
        }
    }
    return glSupported;
}

bool CreateInstance(XrInstance* instance)
{
    // create openxr instance
    XrResult result;
    const char* const enabledExtensions[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};
    XrInstanceCreateInfo ici;
    ici.type = XR_TYPE_INSTANCE_CREATE_INFO;
    ici.next = NULL;
    ici.createFlags = 0;
    ici.enabledExtensionCount = 1;
    ici.enabledExtensionNames = enabledExtensions;
    ici.enabledApiLayerCount = 0;
    ici.enabledApiLayerNames = NULL;
    strcpy(ici.applicationInfo.applicationName, "OpenXR OpenGL Example");
    ici.applicationInfo.engineName[0] = '\0';
    ici.applicationInfo.applicationVersion = 1;
    ici.applicationInfo.engineVersion = 0;
    ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    result = xrCreateInstance(&ici, instance);
    if (!CheckResult(NULL, result, "xrCreateInstance failed"))
    {
        return false;
    }

    bool printRuntimeInfo = false;
    if (printRuntimeInfo)
    {
        XrInstanceProperties ip;
        ip.type = XR_TYPE_INSTANCE_PROPERTIES;
        ip.next = NULL;

        result = xrGetInstanceProperties(*instance, &ip);
        if (!CheckResult(*instance, result, "xrGetInstanceProperties failed"))
        {
            return false;
        }

        printf("Runtime Name: %s\n", ip.runtimeName);
        printf("Runtime Version: %d.%d.%d\n",
               XR_VERSION_MAJOR(ip.runtimeVersion),
               XR_VERSION_MINOR(ip.runtimeVersion),
               XR_VERSION_PATCH(ip.runtimeVersion));
    }

    return true;
}

bool GetSystemId(XrInstance instance, XrSystemId* systemId)
{
    XrResult result;
    XrSystemGetInfo sgi;
    sgi.type = XR_TYPE_SYSTEM_GET_INFO;
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    sgi.next = NULL;

    result = xrGetSystem(instance, &sgi, systemId);
    if (!CheckResult(instance, result, "xrGetSystemFailed"))
    {
        return false;
    }

    bool printSystemProperties = false;
    if (printSystemProperties)
    {
        XrSystemProperties sp;
        sp.type = XR_TYPE_SYSTEM_PROPERTIES;
        sp.next = NULL;
        sp.graphicsProperties = {0};
        sp.trackingProperties = {0};

        result = xrGetSystemProperties(instance, *systemId, &sp);
        if (!CheckResult(instance, result, "xrGetSystemProperties failed"))
        {
            return false;
        }

        printf("System properties for system \"%s\":\n", sp.systemName);
        printf("    maxLayerCount: %d\n", sp.graphicsProperties.maxLayerCount);
        printf("    maxSwapChainImageHeight: %d\n", sp.graphicsProperties.maxSwapchainImageHeight);
        printf("    maxSwapChainImageWidth: %d\n", sp.graphicsProperties.maxSwapchainImageWidth);
        printf("    Orientation Tracking: %s\n", sp.trackingProperties.orientationTracking ? "true" : "false");
        printf("    Position Tracking: %s\n", sp.trackingProperties.positionTracking ? "true" : "false");
    }

    return true;
}

bool SupportsVR(XrInstance instance, XrSystemId systemId)
{
    XrResult result;
    uint32_t viewConfigurationCount;
    result = xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigurationCount, NULL);
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurations"))
    {
        return false;
    }

    std::vector<XrViewConfigurationType> viewConfigurations(viewConfigurationCount);
    result = xrEnumerateViewConfigurations(instance, systemId, viewConfigurationCount, &viewConfigurationCount, viewConfigurations.data());
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurations"))
    {
        return false;
    }

    XrViewConfigurationType stereoViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    for (uint32_t i = 0; i < viewConfigurationCount; i++)
    {
        XrViewConfigurationProperties vcp;
        vcp.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
        vcp.next = NULL;

        result = xrGetViewConfigurationProperties(instance, systemId, viewConfigurations[i], &vcp);
        if (!CheckResult(instance, result, "xrGetViewConfigurationProperties"))
        {
            return false;
        }

        if (viewConfigurations[i] == stereoViewConfigType && vcp.viewConfigurationType == stereoViewConfigType)
        {
            return true;
        }
    }

    return false;
}

bool EnumerateViews(XrInstance instance, XrSystemId systemId, std::vector<XrViewConfigurationView>* views)
{
    XrResult result;
    uint32_t viewCount;
    XrViewConfigurationType stereoViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    result = xrEnumerateViewConfigurationViews(instance, systemId, stereoViewConfigType, 0, &viewCount, NULL);
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurationViews"))
    {
        return false;
    }

    views->resize(viewCount);
    for (uint32_t i = 0; i < viewCount; i++)
    {
        (*views)[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        (*views)[i].next = NULL;
    }

    result = xrEnumerateViewConfigurationViews(instance, systemId, stereoViewConfigType, viewCount, &viewCount, views->data());
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurationViews"))
    {
        return false;
    }

    bool printViews = true;
    if (printViews)
    {
        printf("views:\n");
        for (uint32_t i = 0; i < viewCount; i++)
        {
            printf("    views[%d]:\n", i);
            printf("        recommendedImageRectWidth: %d\n", (*views)[i].recommendedImageRectWidth);
            printf("        maxImageRectWidth: %d\n", (*views)[i].maxImageRectWidth);
            printf("        recommendedImageRectHeight: %d\n", (*views)[i].recommendedImageRectHeight);
            printf("        maxImageRectHeight: %d\n", (*views)[i].maxImageRectHeight);
            printf("        recommendedSwapchainSampleCount: %d\n", (*views)[i].recommendedSwapchainSampleCount);
            printf("        maxSwapchainSampleCount: %d\n", (*views)[i].maxSwapchainSampleCount);
        }
    }

    return true;
}

bool CreateSession(XrInstance instance, XrSystemId systemId, XrSession* session)
{
    XrResult result;
    // TODO: check if opengl version is sufficient.
    {
        XrGraphicsRequirementsOpenGLKHR reqs;
        reqs.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
        reqs.next = NULL;
        PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
        result = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);
        if (!CheckResult(instance, result, "xrGetInstanceProcAddr"))
        {
            return false;
        }

        result = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &reqs);
        if (!CheckResult(instance, result, "GetOpenGLGraphicsRequirementsPKR"))
        {
            return false;
        }
    }

    XrGraphicsBindingOpenGLWin32KHR glBinding;
    glBinding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
    glBinding.next = NULL;
    glBinding.hDC = wglGetCurrentDC();
    glBinding.hGLRC = wglGetCurrentContext();

    XrSessionCreateInfo sci;
    sci.type = XR_TYPE_SESSION_CREATE_INFO;
    sci.next = &glBinding;
    sci.systemId = systemId;

    result = xrCreateSession(instance, &sci, session);
    if (!CheckResult(instance, result, "xrCreateSession"))
    {
        return false;
    }

    return true;
}

bool CreateStageSpace(XrInstance instance, XrSystemId systemId, XrSession session, XrSpace* stageSpace)
{
    XrResult result;
    bool printReferenceSpaces = true;
    if (printReferenceSpaces) {
        uint32_t referenceSpacesCount;
        result = xrEnumerateReferenceSpaces(session, 0, &referenceSpacesCount, NULL);
        if (!CheckResult(instance, result, "xrEnumerateReferenceSpaces"))
        {
            return false;
        }

        std::vector<XrReferenceSpaceType> referenceSpaces(referenceSpacesCount, XR_REFERENCE_SPACE_TYPE_VIEW);
        result = xrEnumerateReferenceSpaces(session, referenceSpacesCount, &referenceSpacesCount, referenceSpaces.data());
        if (!CheckResult(instance, result, "xrEnumerateReferenceSpaces"))
        {
            return false;
        }

        printf("referenceSpaces:\n");
        for (uint32_t i = 0; i < referenceSpacesCount; i++)
        {
            switch (referenceSpaces[i])
            {
            case XR_REFERENCE_SPACE_TYPE_VIEW:
                printf("    XR_REFERENCE_SPACE_TYPE_VIEW\n");
                break;
            case XR_REFERENCE_SPACE_TYPE_LOCAL:
                printf("    XR_REFERENCE_SPACE_TYPE_LOCAL\n");
                break;
            case XR_REFERENCE_SPACE_TYPE_STAGE:
                printf("    XR_REFERENCE_SPACE_TYPE_STAGE\n");
                break;
            default:
                printf("    XR_REFERENCE_SPACE_TYPE_%d\n", referenceSpaces[i]);
                break;
            }
        }
    }

    XrPosef identityPose;
    identityPose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    identityPose.position = {0.0f, 0.0f, 0.0f};

    XrReferenceSpaceCreateInfo rsci;
    rsci.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    rsci.next = NULL;
    rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    rsci.poseInReferenceSpace = identityPose;

    result = xrCreateReferenceSpace(session, &rsci, stageSpace);
    if (!CheckResult(instance, result, "xrCreateReferenceSpace"))
    {
        return false;
    }

    return true;
}

bool BeginSession(XrInstance instance, XrSystemId systemId, XrSession session)
{
    XrResult result;
    XrViewConfigurationType stereoViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    XrSessionBeginInfo sbi;
    sbi.type = XR_TYPE_SESSION_BEGIN_INFO;
    sbi.next = NULL;
    sbi.primaryViewConfigurationType = stereoViewConfigType;
    result = xrBeginSession(session, &sbi);
    if (!CheckResult(instance, result, "xrBeginSession"))
    {
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (!GLSupport())
    {
        printf("XR_KHR_OPENGL_ENABLE_EXTENSION not supported!\n");
        return 1;
    }

    XrInstance instance;
    if (!CreateInstance(&instance))
    {
        return 1;
    }

    XrSystemId systemId;
    if (!GetSystemId(instance, &systemId))
    {
        return 1;
    }

    if (!SupportsVR(instance, systemId))
    {
        printf("System doesn't support VR\n");
        return 1;
    }

    std::vector<XrViewConfigurationView> views;
    if (!EnumerateViews(instance, systemId, &views))
    {
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) != 0)
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("openxrstub", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 512, SDL_WINDOW_OPENGL);

    gl_context = SDL_GL_CreateContext(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        SDL_Log("Failed to SDL Renderer: %s", SDL_GetError());
        return -1;
    }

    SDL_AddEventWatch(watch, NULL);

    XrSession session;
    if (!CreateSession(instance, systemId, &session))
    {
        return 1;
    }

    XrSpace stageSpace;
    if (!CreateStageSpace(instance, systemId, session, &stageSpace))
    {
        return 1;
    }

    if (!BeginSession(instance, systemId, session))
    {
        return 1;
    }

    while (!quitting)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event) )
        {
            if (event.type == SDL_QUIT)
            {
                quitting = true;
            }
        }

        render();
        SDL_Delay(2);
    }

    SDL_DelEventWatch(watch, NULL);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    xrDestroySpace(stageSpace);
    xrEndSession(session);
    xrDestroySession(session);
    xrDestroyInstance(instance);

    return 0;
}
