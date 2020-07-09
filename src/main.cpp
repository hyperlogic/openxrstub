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
#include <array>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <stdlib.h> //rand()
#include <cassert>

static bool quitting = false;
static float r = 0.0f;
static SDL_Window *window = NULL;
static SDL_GLContext gl_context;
static SDL_Renderer *renderer = NULL;

bool printAll = true;

struct Context
{
    std::vector<XrExtensionProperties> extensionProps;
    std::vector<XrApiLayerProperties> layerProps;
    std::vector<XrViewConfigurationView> viewConfigs;

    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrActionSet actionSet = XR_NULL_HANDLE;
    XrSpace stageSpace = XR_NULL_HANDLE;

    struct SwapchainInfo
    {
        XrSwapchain handle;
        int32_t width;
        int32_t height;
    };
    std::vector<SwapchainInfo> swapchains;
    std::vector<std::vector<XrSwapchainImageOpenGLKHR>> swapchainImages;

    std::vector<GLuint> frameBuffers;
    std::vector<GLuint> depthBuffers;
};

void render()
{
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

    if (instance != XR_NULL_HANDLE)
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

bool EnumerateExtensions(std::vector<XrExtensionProperties>& extensionProps)
{
    XrResult result;
    uint32_t extensionCount = 0;
    result = xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);
    if (!CheckResult(NULL, result, "xrEnumerateInstanceExtensionProperties failed"))
    {
        return false;
    }
    extensionProps.resize(extensionCount);
    for (uint32_t i = 0; i < extensionCount; i++) {
        extensionProps[i].type = XR_TYPE_EXTENSION_PROPERTIES;
        extensionProps[i].next = NULL;
    }
    result = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, extensionProps.data());
    if (!CheckResult(NULL, result, "xrEnumerateInstanceExtensionProperties failed"))
    {
        return false;
    }

    bool printExtensions = false;
    if (printExtensions || printAll)
    {
        printf("%d extensions:\n", extensionCount);
        for (uint32_t i = 0; i < extensionCount; ++i)
        {
            printf("    %s\n", extensionProps[i].extensionName);
        }
    }

    return true;
}

bool ExtensionSupported(const std::vector<XrExtensionProperties>& extensions, const char* extensionName)
{
    bool supported = false;
    for (auto& extension : extensions)
    {
        if (!strcmp(extensionName, extension.extensionName))
        {
            supported = true;
        }
    }
    return supported;
}

bool EnumerateLayers(std::vector<XrApiLayerProperties>& layerProps)
{
    uint32_t layerCount;
    XrResult result = xrEnumerateApiLayerProperties(0, &layerCount, NULL);
    if (!CheckResult(NULL, result, "xrEnumerateApiLayerProperties"))
    {
        return false;
    }

    layerProps.resize(layerCount);
    for (uint32_t i = 0; i < layerCount; i++) {
        layerProps[i].type = XR_TYPE_API_LAYER_PROPERTIES;
        layerProps[i].next = NULL;
    }
    result = xrEnumerateApiLayerProperties(layerCount, &layerCount, layerProps.data());
    if (!CheckResult(NULL, result, "xrEnumerateApiLayerProperties"))
    {
        return false;
    }

    bool printLayers = false;
    if (printLayers || printAll)
    {
        printf("%d layers:\n", layerCount);
        for (uint32_t i = 0; i < layerCount; i++)
        {
            printf("    %s, %s\n", layerProps[i].layerName, layerProps[i].description);
        }
    }

    return true;
}

bool CreateInstance(XrInstance& instance)
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

    result = xrCreateInstance(&ici, &instance);
    if (!CheckResult(NULL, result, "xrCreateInstance failed"))
    {
        return false;
    }

    bool printRuntimeInfo = false;
    if (printRuntimeInfo || printAll)
    {
        XrInstanceProperties ip;
        ip.type = XR_TYPE_INSTANCE_PROPERTIES;
        ip.next = NULL;

        result = xrGetInstanceProperties(instance, &ip);
        if (!CheckResult(instance, result, "xrGetInstanceProperties failed"))
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

bool GetSystemId(XrInstance instance, XrSystemId& systemId)
{
    XrResult result;
    XrSystemGetInfo sgi;
    sgi.type = XR_TYPE_SYSTEM_GET_INFO;
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    sgi.next = NULL;

    result = xrGetSystem(instance, &sgi, &systemId);
    if (!CheckResult(instance, result, "xrGetSystemFailed"))
    {
        return false;
    }

    bool printSystemProperties = false;
    if (printSystemProperties || printAll)
    {
        XrSystemProperties sp;
        sp.type = XR_TYPE_SYSTEM_PROPERTIES;
        sp.next = NULL;
        sp.graphicsProperties = {0};
        sp.trackingProperties = {0};

        result = xrGetSystemProperties(instance, systemId, &sp);
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

bool EnumerateViewConfigs(XrInstance instance, XrSystemId systemId, std::vector<XrViewConfigurationView>& viewConfigs)
{
    XrResult result;
    uint32_t viewCount;
    XrViewConfigurationType stereoViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    result = xrEnumerateViewConfigurationViews(instance, systemId, stereoViewConfigType, 0, &viewCount, NULL);
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurationViews"))
    {
        return false;
    }

    viewConfigs.resize(viewCount);
    for (uint32_t i = 0; i < viewCount; i++)
    {
        viewConfigs[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        viewConfigs[i].next = NULL;
    }

    result = xrEnumerateViewConfigurationViews(instance, systemId, stereoViewConfigType, viewCount, &viewCount, viewConfigs.data());
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurationViews"))
    {
        return false;
    }

    bool printViews = false;
    if (printViews || printAll)
    {
        printf("%d viewConfigs:\n", viewCount);
        for (uint32_t i = 0; i < viewCount; i++)
        {
            printf("    viewConfigs[%d]:\n", i);
            printf("        recommendedImageRectWidth: %d\n", viewConfigs[i].recommendedImageRectWidth);
            printf("        maxImageRectWidth: %d\n", viewConfigs[i].maxImageRectWidth);
            printf("        recommendedImageRectHeight: %d\n", viewConfigs[i].recommendedImageRectHeight);
            printf("        maxImageRectHeight: %d\n", viewConfigs[i].maxImageRectHeight);
            printf("        recommendedSwapchainSampleCount: %d\n", viewConfigs[i].recommendedSwapchainSampleCount);
            printf("        maxSwapchainSampleCount: %d\n", viewConfigs[i].maxSwapchainSampleCount);
        }
    }

    return true;
}

bool CreateSession(XrInstance instance, XrSystemId systemId, XrSession& session)
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

    result = xrCreateSession(instance, &sci, &session);
    if (!CheckResult(instance, result, "xrCreateSession"))
    {
        return false;
    }

    return true;
}

bool CreateActions(XrInstance instance, XrSystemId systemId, XrSession session, XrActionSet& actionSet)
{
    XrResult result;

    // create action set
    XrActionSetCreateInfo asci;
    asci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    asci.next = NULL;
    strcpy_s(asci.actionSetName, "gameplay");
    strcpy_s(asci.localizedActionSetName, "Gameplay");
    asci.priority = 0;
    result = xrCreateActionSet(instance, &asci, &actionSet);
    if (!CheckResult(instance, result, "xrCreateActionSet"))
    {
        return false;
    }

    std::array<XrPath, 2> handPath = {XR_NULL_PATH, XR_NULL_PATH};
    xrStringToPath(instance, "/user/hand/left", handPath.data() + 0);
    xrStringToPath(instance, "/user/hand/right", handPath.data() + 1);
    if (!CheckResult(instance, result, "xrStringToPath"))
    {
        return false;
    }

    XrAction grabAction = XR_NULL_HANDLE;
    XrActionCreateInfo aci;
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    aci.next = NULL;
    aci.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    strcpy_s(aci.actionName, "grab_object");
    strcpy_s(aci.localizedActionName, "Grab Object");
    aci.countSubactionPaths = 2;
    aci.subactionPaths = handPath.data();
    result = xrCreateAction(actionSet, &aci, &grabAction);
    if (!CheckResult(instance, result, "xrCreateAction"))
    {
        return false;
    }

    XrAction poseAction = XR_NULL_HANDLE;
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    aci.next = NULL;
    aci.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy_s(aci.actionName, "hand_pose");
    strcpy_s(aci.localizedActionName, "Hand Pose");
    aci.countSubactionPaths = 2;
    aci.subactionPaths = handPath.data();
    result = xrCreateAction(actionSet, &aci, &poseAction);
    if (!CheckResult(instance, result, "xrCreateAction"))
    {
        return false;
    }

    XrAction vibrateAction = XR_NULL_HANDLE;
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    aci.next = NULL;
    aci.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
    strcpy_s(aci.actionName, "vibrate_hand");
    strcpy_s(aci.localizedActionName, "Vibrate Hand");
    aci.countSubactionPaths = 2;
    aci.subactionPaths = handPath.data();
    result = xrCreateAction(actionSet, &aci, &vibrateAction);
    if (!CheckResult(instance, result, "xrCreateAction"))
    {
        return false;
    }

    XrAction quitAction = XR_NULL_HANDLE;
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    aci.next = NULL;
    aci.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy_s(aci.actionName, "quit_session");
    strcpy_s(aci.localizedActionName, "Quit Session");
    aci.countSubactionPaths = 2;
    aci.subactionPaths = handPath.data();
    result = xrCreateAction(actionSet, &aci, &quitAction);
    if (!CheckResult(instance, result, "xrCreateAction"))
    {
        return false;
    }

    std::array<XrPath, 2> selectPath = {XR_NULL_PATH, XR_NULL_PATH};
    std::array<XrPath, 2> squeezeValuePath = {XR_NULL_PATH, XR_NULL_PATH};
    std::array<XrPath, 2> squeezeClickPath = {XR_NULL_PATH, XR_NULL_PATH};
    std::array<XrPath, 2> posePath = {XR_NULL_PATH, XR_NULL_PATH};
    std::array<XrPath, 2> hapticPath = {XR_NULL_PATH, XR_NULL_PATH};
    std::array<XrPath, 2> menuClickPath = {XR_NULL_PATH, XR_NULL_PATH};
    xrStringToPath(instance, "/user/hand/left/input/select/click", selectPath.data() + 0);
    xrStringToPath(instance, "/user/hand/right/input/select/click", selectPath.data() + 1);
    xrStringToPath(instance, "/user/hand/left/input/squeeze/value", squeezeValuePath.data() + 0);
    xrStringToPath(instance, "/user/hand/right/input/squeeze/value", squeezeValuePath.data() + 1);
    xrStringToPath(instance, "/user/hand/left/input/squeeze/click", squeezeClickPath.data() + 0);
    xrStringToPath(instance, "/user/hand/right/input/squeeze/click", squeezeClickPath.data() + 1);
    xrStringToPath(instance, "/user/hand/left/input/grip/pose", posePath.data() + 0);
    xrStringToPath(instance, "/user/hand/right/input/grip/pose", posePath.data() + 1);
    xrStringToPath(instance, "/user/hand/left/output/haptic", hapticPath.data() + 0);
    xrStringToPath(instance, "/user/hand/right/output/haptic", hapticPath.data() + 1);
    xrStringToPath(instance, "/user/hand/left/input/menu/click", menuClickPath.data() + 0);
    xrStringToPath(instance, "/user/hand/right/input/menu/click", menuClickPath.data() + 1);
    if (!CheckResult(instance, result, "xrStringToPath"))
    {
        return false;
    }

    // KHR Simple
    {
        XrPath interactionProfilePath = XR_NULL_PATH;
        xrStringToPath(instance, "/interaction_profiles/khr/simple_controller", &interactionProfilePath);
        std::vector<XrActionSuggestedBinding> bindings = {
            {grabAction, selectPath[0]},
            {grabAction, selectPath[1]},
            {poseAction, posePath[0]},
            {poseAction, posePath[1]},
            {quitAction, menuClickPath[0]},
            {quitAction, menuClickPath[1]},
            {vibrateAction, hapticPath[0]},
            {vibrateAction, hapticPath[1]}
        };

        XrInteractionProfileSuggestedBinding suggestedBindings;
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.next = NULL;
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (!CheckResult(instance, result, "xrSuggestInteractionProfileBindings"))
        {
            return false;
        }
    }

    // oculus touch
    {
        XrPath interactionProfilePath = XR_NULL_PATH;
        xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);
        std::vector<XrActionSuggestedBinding> bindings = {
            // AJT: WTF these result in errors
            {grabAction, squeezeValuePath[0]},




            {grabAction, squeezeValuePath[1]},
            {poseAction, posePath[0]},
            {poseAction, posePath[1]},
            {quitAction, menuClickPath[0]},
            //{quitAction, menuClickPath[1]},  // no menu button on right controller?
            {vibrateAction, hapticPath[0]},
            {vibrateAction, hapticPath[1]}
        };

        XrInteractionProfileSuggestedBinding suggestedBindings;
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.next = NULL;
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (!CheckResult(instance, result, "xrSuggestInteractionProfileBindings (oculus)"))
        {
            return false;
        }
    }

    // vive
    {
        XrPath interactionProfilePath = XR_NULL_PATH;
        xrStringToPath(instance, "/interaction_profiles/htc/vive_controller", &interactionProfilePath);
        std::vector<XrActionSuggestedBinding> bindings = {
            {grabAction, squeezeClickPath[0]},
            {grabAction, squeezeClickPath[1]},
            {poseAction, posePath[0]},
            {poseAction, posePath[1]},
            {quitAction, menuClickPath[0]},
            {quitAction, menuClickPath[1]},
            {vibrateAction, hapticPath[0]},
            {vibrateAction, hapticPath[1]}
        };

        XrInteractionProfileSuggestedBinding suggestedBindings;
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.next = NULL;
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (!CheckResult(instance, result, "xrSuggestInteractionProfileBindings (vive)"))
        {
            return false;
        }
    }

    // microsoft mixed reality
    {
        XrPath interactionProfilePath = XR_NULL_PATH;
        xrStringToPath(instance, "/interaction_profiles/microsoft/motion_controller", &interactionProfilePath);
        std::vector<XrActionSuggestedBinding> bindings = {
            {grabAction, squeezeClickPath[0]},
            {grabAction, squeezeClickPath[1]},
            {poseAction, posePath[0]},
            {poseAction, posePath[1]},
            {quitAction, menuClickPath[0]},
            {quitAction, menuClickPath[1]},
            {vibrateAction, hapticPath[0]},
            {vibrateAction, hapticPath[1]}
        };

        XrInteractionProfileSuggestedBinding suggestedBindings;
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.next = NULL;
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
        if (!CheckResult(instance, result, "xrSuggestInteractionProfileBindings"))
        {
            return false;
        }
    }

    std::array<XrSpace, 2> handSpace = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrActionSpaceCreateInfo aspci;
    aspci.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    aspci.next = NULL;
    aspci.action = poseAction;
    XrPosef identity;
    identity.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    identity.position = {0.0f, 0.0f, 0.0f};
    aspci.poseInActionSpace = identity;
    aspci.subactionPath = handPath[0];
    result = xrCreateActionSpace(session, &aspci, handSpace.data() + 0);
    if (!CheckResult(instance, result, "xrCreateActionSpace"))
    {
        return false;
    }

    aspci.subactionPath = handPath[1];
    result = xrCreateActionSpace(session, &aspci, handSpace.data() + 1);
    if (!CheckResult(instance, result, "xrCreateActionSpace"))
    {
        return false;
    }

    XrSessionActionSetsAttachInfo sasai;
    sasai.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    sasai.next = NULL;
    sasai.countActionSets = 1;
    sasai.actionSets = &actionSet;
    result = xrAttachSessionActionSets(session, &sasai);
    if (!CheckResult(instance, result, "xrSessionActionSetsAttachInfo"))
    {
        return false;
    }

    return true;
}

bool CreateStageSpace(XrInstance instance, XrSystemId systemId, XrSession session, XrSpace& stageSpace)
{
    XrResult result;
    bool printReferenceSpaces = true;
    if (printReferenceSpaces || printAll)
    {
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

    result = xrCreateReferenceSpace(session, &rsci, &stageSpace);
    if (!CheckResult(instance, result, "xrCreateReferenceSpace"))
    {
        return false;
    }

    return true;
}

bool CreateSwapchains(Context& context)
{
    XrResult result;
    uint32_t swapchainFormatCount;
    result = xrEnumerateSwapchainFormats(context.session, 0, &swapchainFormatCount, NULL);
    if (!CheckResult(context.instance, result, "xrEnumerateSwapchainFormats"))
    {
        return false;
    }

    std::vector<int64_t> swapchainFormats(swapchainFormatCount);
    result = xrEnumerateSwapchainFormats(context.session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data());
    if (!CheckResult(context.instance, result, "xrEnumerateSwapchainFormats"))
    {
        return false;
    }

    // TODO: pick a format.
    int64_t swapchainFormatToUse = swapchainFormats[0];

    std::vector<uint32_t> swapchainLengths(context.viewConfigs.size());

    context.swapchains.resize(context.viewConfigs.size());

    for (uint32_t i = 0; i < context.viewConfigs.size(); i++)
    {
        XrSwapchainCreateInfo sci;
        sci.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        sci.next = NULL;
        sci.createFlags = 0;
        sci.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        sci.format = swapchainFormatToUse;
        sci.sampleCount = 1;
        sci.width = context.viewConfigs[i].recommendedImageRectWidth;
        sci.height = context.viewConfigs[i].recommendedImageRectHeight;
        sci.faceCount = 1;
        sci.arraySize = 1;
        sci.mipCount = 1;

        XrSwapchain swapchainHandle;
        result = xrCreateSwapchain(context.session, &sci, &swapchainHandle);
        if (!CheckResult(context.instance, result, "xrCreateSwapchain"))
        {
            return false;
        }

        context.swapchains[i].handle = swapchainHandle;
        context.swapchains[i].width = sci.width;
        context.swapchains[i].height = sci.height;

        result = xrEnumerateSwapchainImages(context.swapchains[i].handle, 0, swapchainLengths.data() + i, NULL);
        if (!CheckResult(context.instance, result, "xrEnumerateSwapchainImages"))
        {
            return false;
        }
    }

    context.swapchainImages.resize(context.viewConfigs.size());
    for (uint32_t i = 0; i < context.viewConfigs.size(); i++)
    {
        context.swapchainImages[i].resize(swapchainLengths[i]);
        for (uint32_t j = 0; j < swapchainLengths[i]; j++)
        {
            context.swapchainImages[i][j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
            context.swapchainImages[i][j].next = NULL;
        }

        result = xrEnumerateSwapchainImages(context.swapchains[i].handle, swapchainLengths[i], &swapchainLengths[i],
                                            (XrSwapchainImageBaseHeader*)(context.swapchainImages[i].data()));
        if (!CheckResult(context.instance, result, "xrEnumerateSwapchainImages"))
        {
            return false;
        }
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

bool CreateFrameBuffers(const std::vector<XrViewConfigurationView>& viewConfig,
                        std::vector<GLuint>& frameBuffers, std::vector<GLuint>& depthBuffers)
{
    frameBuffers.resize(viewConfig.size(), 0);
    depthBuffers.resize(viewConfig.size(), 0);
    for (size_t i = 0; i < viewConfig.size(); i++)
    {
        glGenRenderbuffers(1, depthBuffers.data() + i);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffers[i]);
        uint32_t sampleCount = viewConfig[i].recommendedSwapchainSampleCount;
        uint32_t width = viewConfig[i].recommendedImageRectWidth;
        uint32_t height = viewConfig[i].recommendedImageRectHeight;
        if (sampleCount == 1)
        {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        }
        else
        {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_DEPTH24_STENCIL8, width, height);
        }
        glGenFramebuffers(1, frameBuffers.data() + i);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffers[i]);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffers[i]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    return true;
}

bool SyncInput(Context& context)
{
    XrResult result;

    // syncInput
    XrActiveActionSet aas;
    aas.actionSet = context.actionSet;
    aas.subactionPath = XR_NULL_PATH;
    XrActionsSyncInfo asi;
    asi.type = XR_TYPE_ACTIONS_SYNC_INFO;
    asi.next = NULL;
    asi.countActiveActionSets = 1;
    asi.activeActionSets = &aas;
    result = xrSyncActions(context.session, &asi);
    if (!CheckResult(context.instance, result, "xrSyncActions"))
    {
        return false;
    }

    // AJT: TODO xrGetActionStateFloat()
    // xrGetActionStatePose()

    return true;
}

bool RenderView(const XrCompositionLayerProjectionView& layerView,
                const XrSwapchainImageOpenGLKHR& swapchainImage,
                GLuint frameBuffer, GLuint depthBuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
               static_cast<GLint>(layerView.subImage.imageRect.offset.y),
               static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
               static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));

    const uint32_t colorTexture = swapchainImage.image;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // render shit

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

bool RenderLayer(Context& context, XrTime predictedDisplayTime,
                 std::vector<XrCompositionLayerProjectionView>& projectionLayerViews,
                 XrCompositionLayerProjection& layer)
{
    XrViewState viewState;
    viewState.type = XR_TYPE_VIEW_STATE;
    viewState.next = NULL;

    uint32_t viewCapacityInput = (uint32_t)context.viewConfigs.size();
    uint32_t viewCountOutput;

    std::vector<XrView> views(context.viewConfigs.size());
    for (size_t i = 0; i < context.viewConfigs.size(); i++)
    {
        views[i].type = XR_TYPE_VIEW;
        views[i].next = NULL;
    }

    XrViewLocateInfo vli;
    vli.type = XR_TYPE_VIEW_LOCATE_INFO;
    vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    vli.displayTime = predictedDisplayTime;
    vli.space = context.stageSpace;
    XrResult result = xrLocateViews(context.session, &vli, &viewState, viewCapacityInput, &viewCountOutput, views.data());
    if (!CheckResult(context.instance, result, "xrLocateViews"))
    {
        return false;
    }

    if (XR_UNQUALIFIED_SUCCESS(result))
    {
        assert(viewCountOutput == viewCapacityInput);
        assert(viewCountOutput == context.viewConfigs.size());
        assert(viewCountOutput == context.swapchains.size());

        projectionLayerViews.resize(viewCountOutput);

        // Render view to the appropriate part of the swapchain image.
        for (uint32_t i = 0; i < viewCountOutput; i++)
        {
            // Each view has a separate swapchain which is acquired, rendered to, and released.
            const Context::SwapchainInfo viewSwapchain = context.swapchains[i];

            XrSwapchainImageAcquireInfo ai;
            ai.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
            ai.next = NULL;

            uint32_t swapchainImageIndex;
            result = xrAcquireSwapchainImage(viewSwapchain.handle, &ai, &swapchainImageIndex);
            if (!CheckResult(context.instance, result, "xrAquireSwapchainImage"))
            {
                return false;
            }

            XrSwapchainImageWaitInfo wi;
            wi.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
            wi.next = NULL;
            wi.timeout = XR_INFINITE_DURATION;
            result = xrWaitSwapchainImage(viewSwapchain.handle, &wi);
            if (!CheckResult(context.instance, result, "xrWaitSwapchainImage"))
            {
                return false;
            }

            projectionLayerViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            projectionLayerViews[i].pose = views[i].pose;
            projectionLayerViews[i].fov = views[i].fov;
            projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
            projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
            projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

            const XrSwapchainImageOpenGLKHR& swapchainImage = context.swapchainImages[i][swapchainImageIndex];
            RenderView(projectionLayerViews[i], swapchainImage, context.frameBuffers[i], context.depthBuffers[i]);

            XrSwapchainImageReleaseInfo ri;
            ri.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
            ri.next = NULL;
            result = xrReleaseSwapchainImage(viewSwapchain.handle, &ri);
            if (!CheckResult(context.instance, result, "xrReleaseSwapchainImage"))
            {
                return false;
            }

            layer.space = context.stageSpace;
            layer.viewCount = (uint32_t)projectionLayerViews.size();
            layer.views = projectionLayerViews.data();
        }
    }

    return true;
}

bool RenderFrame(Context& context)
{
    XrFrameState fs;
    fs.type = XR_TYPE_FRAME_STATE;
    fs.next = NULL;

    XrFrameWaitInfo fwi;
    fwi.type = XR_TYPE_FRAME_WAIT_INFO;
    fwi.next = NULL;

    XrResult result = xrWaitFrame(context.session, &fwi, &fs);
    if (!CheckResult(context.instance, result, "xrWaitFrame"))
    {
        return false;
    }

    printf("after xrWaitFrame\n");

    XrFrameBeginInfo fbi;
    fbi.type = XR_TYPE_FRAME_BEGIN_INFO;
    fbi.next = NULL;
    result = xrBeginFrame(context.session, &fbi);
    if (!CheckResult(context.instance, result, "xrBeginFrame"))
    {
        return false;
    }

    printf("after xrBeginFrame\n");

    std::vector<XrCompositionLayerBaseHeader*> layers;
    XrCompositionLayerProjection layer;
    layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    layer.next = NULL;

    std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
    if (fs.shouldRender == XR_TRUE)
    {
        if (RenderLayer(context, fs.predictedDisplayTime, projectionLayerViews, layer))
        {
            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
        }
    }

    XrFrameEndInfo fei;
    fei.type = XR_TYPE_FRAME_END_INFO;
    fei.next = NULL;
    fei.displayTime = fs.predictedDisplayTime;
    fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    fei.layerCount = (uint32_t)layers.size();
    fei.layers = layers.data();
    result = xrEndFrame(context.session, &fei);
    if (!CheckResult(context.instance, result, "xrEndFrame"))
    {
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    Context context;
    if (!EnumerateExtensions(context.extensionProps))
    {
        return 1;
    }

    if (!ExtensionSupported(context.extensionProps, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME))
    {
        printf("XR_KHR_opengl_enable not supported!\n");
        return 1;
    }

    if (!EnumerateLayers(context.layerProps))
    {
        return 1;
    }

    if (!CreateInstance(context.instance))
    {
        return 1;
    }

    if (!GetSystemId(context.instance, context.systemId))
    {
        return 1;
    }

    if (!SupportsVR(context.instance, context.systemId))
    {
        printf("System doesn't support VR\n");
        return 1;
    }

    if (!EnumerateViewConfigs(context.instance, context.systemId, context.viewConfigs))
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

    SDL_GL_MakeCurrent(window, gl_context);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        printf("glewInit failed: %s\n", glewGetErrorString(err));
        return 1;
    }

    SDL_AddEventWatch(watch, NULL);

    if (!CreateSession(context.instance, context.systemId, context.session))
    {
        return 1;
    }

    if (!CreateActions(context.instance, context.systemId, context.session, context.actionSet))
    {
        return 1;
    }

    if (!CreateStageSpace(context.instance, context.systemId, context.session, context.stageSpace))
    {
        return 1;
    }

    if (!CreateSwapchains(context))
    {
        return 1;
    }

    if (!CreateFrameBuffers(context.viewConfigs, context.frameBuffers, context.depthBuffers))
    {
        return 1;
    }

    bool sessionReady = false;
    XrSessionState xrState = XR_SESSION_STATE_UNKNOWN;
    while (!quitting)
    {
        XrEventDataBuffer xrEvent;
        xrEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
        xrEvent.next = NULL;

        XrResult result = xrPollEvent(context.instance, &xrEvent);
        if (result == XR_SUCCESS)
        {
            switch (xrEvent.type)
            {
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                // Receiving the XrEventDataInstanceLossPending event structure indicates that the application is about to lose the indicated XrInstance at the indicated lossTime in the future.
                // The application should call xrDestroyInstance and relinquish any instance-specific resources.
                // This typically occurs to make way for a replacement of the underlying runtime, such as via a software update.
                printf("xrEvent: XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING\n");
                break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            {
                // Receiving the XrEventDataSessionStateChanged event structure indicates that the application has changed lifecycle stat.e
                printf("xrEvent: XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED -> ");
                XrEventDataSessionStateChanged* ssc = (XrEventDataSessionStateChanged*)&xrEvent;
                xrState = ssc->state;
                switch (xrState)
                {
                case XR_SESSION_STATE_IDLE:
                    // The initial state after calling xrCreateSession or returned to after calling xrEndSession.
                    printf("XR_SESSION_STATE_IDLE\n");
                    break;
                case XR_SESSION_STATE_READY:
                    // The application is ready to call xrBeginSession and sync its frame loop with the runtime.
                    printf("XR_SESSION_STATE_READY\n");
                    if (!BeginSession(context.instance, context.systemId, context.session))
                    {
                        return 1;
                    }
                    sessionReady = true;
                    break;
                case XR_SESSION_STATE_SYNCHRONIZED:
                    // The application has synced its frame loop with the runtime but is not visible to the user.
                    printf("XR_SESSION_STATE_SYNCHRONIZED\n");
                    break;
                case XR_SESSION_STATE_VISIBLE:
                    // The application has synced its frame loop with the runtime and is visible to the user but cannot receive XR input.
                    printf("XR_SESSION_STATE_VISIBLE\n");
                    break;
                case XR_SESSION_STATE_FOCUSED:
                    // The application has synced its frame loop with the runtime, is visible to the user and can receive XR input.
                    printf("XR_SESSION_STATE_FOCUSED\n");
                    break;
                case XR_SESSION_STATE_STOPPING:
                    // The application should exit its frame loop and call xrEndSession.
                    printf("XR_SESSION_STATE_STOPPING\n");
                    break;
                case XR_SESSION_STATE_LOSS_PENDING:
                    printf("XR_SESSION_STATE_LOSS_PENDING\n");
                    // The session is in the process of being lost. The application should destroy the current session and can optionally recreate it.
                    break;
                case XR_SESSION_STATE_EXITING:
                    printf("XR_SESSION_STATE_EXITING\n");
                    // The application should end its XR experience and not automatically restart it.
                    break;
                default:
                    printf("XR_SESSION_STATE_??? %d\n", (int)xrState);
                    break;
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                // The XrEventDataReferenceSpaceChangePending event is sent to the application to notify it that the origin (and perhaps the bounds) of a reference space is changing.
                printf("XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING\n");
                break;
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                // Receiving the XrEventDataEventsLost event structure indicates that the event queue overflowed and some events were removed at the position within the queue at which this event was found.
                printf("xrEvent: XR_TYPE_EVENT_DATA_EVENTS_LOST\n");
                break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                // The XrEventDataInteractionProfileChanged event is sent to the application to notify it that the active input form factor for one or more top level user paths has changed.:
                printf("XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED\n");
                break;
            default:
                printf("Unhandled event type %d\n", xrEvent.type);
                break;
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quitting = true;
            }
        }

        if (sessionReady)
        {
            static int frameNumber = 0;
            printf("frame = %d\n", frameNumber);
            frameNumber++;

            if (!SyncInput(context))
            {
                return 1;
            }

            if (!RenderFrame(context))
            {
                return 1;
            }
        }
        else
        {
            SDL_Delay(100);
        }

        //render();
        //SDL_Delay(2);
    }

    SDL_DelEventWatch(watch, NULL);
    SDL_GL_DeleteContext(gl_context);

    for (auto& frameBuffer : context.frameBuffers)
    {
        glDeleteFramebuffers(1, &frameBuffer);
    }

    for (auto& depthBuffer : context.depthBuffers)
    {
        glDeleteRenderbuffers(1, &depthBuffer);
    }

    XrResult result;
    for (auto& swapchain : context.swapchains)
    {
        result = xrDestroySwapchain(swapchain.handle);
        CheckResult(context.instance, result, "xrDestroySwapchain");
    }

    result = xrDestroySpace(context.stageSpace);
    CheckResult(context.instance, result, "xrDestroySpace");

    xrEndSession(context.session);
    CheckResult(context.instance, result, "xrEndSession");

    xrDestroySession(context.session);
    CheckResult(context.instance, result, "xrDestroySession");

    xrDestroyInstance(context.instance);
    CheckResult(XR_NULL_HANDLE, result, "xrDestroyInstance");

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
