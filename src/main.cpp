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

static bool quitting = false;
static float r = 0.0f;
static SDL_Window *window = NULL;
static SDL_GLContext gl_context;
static SDL_Renderer *renderer = NULL;

bool printAll = true;

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

bool SupportsGL()
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
    if (printExtensions || printAll)
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

        if (printExtensions || printAll)
        {
            printf("    %s\n", extensionProperties[i].extensionName);
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
    if (printRuntimeInfo || printAll)
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
    if (printSystemProperties || printAll)
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

bool EnumerateViews(XrInstance instance, XrSystemId systemId, std::vector<XrViewConfigurationView>* viewConfigs)
{
    XrResult result;
    uint32_t viewCount;
    XrViewConfigurationType stereoViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    result = xrEnumerateViewConfigurationViews(instance, systemId, stereoViewConfigType, 0, &viewCount, NULL);
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurationViews"))
    {
        return false;
    }

    viewConfigs->resize(viewCount);
    for (uint32_t i = 0; i < viewCount; i++)
    {
        (*viewConfigs)[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        (*viewConfigs)[i].next = NULL;
    }

    result = xrEnumerateViewConfigurationViews(instance, systemId, stereoViewConfigType, viewCount, &viewCount, viewConfigs->data());
    if (!CheckResult(instance, result, "xrEnumerateViewConfigurationViews"))
    {
        return false;
    }

    bool printViews = false;
    if (printViews || printAll)
    {
        printf("viewConfigs:\n");
        for (uint32_t i = 0; i < viewCount; i++)
        {
            printf("    viewConfigs[%d]:\n", i);
            printf("        recommendedImageRectWidth: %d\n", (*viewConfigs)[i].recommendedImageRectWidth);
            printf("        maxImageRectWidth: %d\n", (*viewConfigs)[i].maxImageRectWidth);
            printf("        recommendedImageRectHeight: %d\n", (*viewConfigs)[i].recommendedImageRectHeight);
            printf("        maxImageRectHeight: %d\n", (*viewConfigs)[i].maxImageRectHeight);
            printf("        recommendedSwapchainSampleCount: %d\n", (*viewConfigs)[i].recommendedSwapchainSampleCount);
            printf("        maxSwapchainSampleCount: %d\n", (*viewConfigs)[i].maxSwapchainSampleCount);
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

bool CreateActions(XrInstance instance, XrSystemId systemId, XrSession session, XrActionSet* actionSet)
{
    XrResult result;

    // create action set
    XrActionSetCreateInfo asci;
    asci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    asci.next = NULL;
    strcpy_s(asci.actionSetName, "gameplay");
    strcpy_s(asci.localizedActionSetName, "Gameplay");
    asci.priority = 0;
    result = xrCreateActionSet(instance, &asci, actionSet);
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
    result = xrCreateAction(*actionSet, &aci, &grabAction);
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
    result = xrCreateAction(*actionSet, &aci, &poseAction);
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
    result = xrCreateAction(*actionSet, &aci, &vibrateAction);
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
    result = xrCreateAction(*actionSet, &aci, &quitAction);
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
    sasai.actionSets = actionSet;
    result = xrAttachSessionActionSets(session, &sasai);
    if (!CheckResult(instance, result, "xrSessionActionSetsAttachInfo"))
    {
        return false;
    }

    return true;
}

bool CreateStageSpace(XrInstance instance, XrSystemId systemId, XrSession session, XrSpace* stageSpace)
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

    result = xrCreateReferenceSpace(session, &rsci, stageSpace);
    if (!CheckResult(instance, result, "xrCreateReferenceSpace"))
    {
        return false;
    }

    return true;
}

bool CreateSwapchains(XrInstance instance, XrSystemId systemId, XrSession session,
                      const std::vector<XrViewConfigurationView>& viewConfigs,
                      std::vector<XrSwapchain>* swapchains,
                      std::vector<std::vector<XrSwapchainImageOpenGLKHR>>* swapchainImages)
{
    XrResult result;
    uint32_t swapchainFormatCount;
    result = xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, NULL);
    if (!CheckResult(instance, result, "xrEnumerateSwapchainFormats"))
    {
        return false;
    }

    std::vector<int64_t> swapchainFormats(swapchainFormatCount);
    result = xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data());
    if (!CheckResult(instance, result, "xrEnumerateSwapchainFormats"))
    {
        return false;
    }

    // TODO: pick a format.
    int64_t swapchainFormatToUse = swapchainFormats[0];

    std::vector<uint32_t> swapchainLengths(viewConfigs.size());

    swapchains->resize(viewConfigs.size());

    for (uint32_t i = 0; i < viewConfigs.size(); i++)
    {
        XrSwapchainCreateInfo sci;
        sci.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        sci.next = NULL;
        sci.createFlags = 0;
        sci.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        sci.format = swapchainFormatToUse;
        sci.sampleCount = 1;
        sci.width = viewConfigs[i].recommendedImageRectWidth;
        sci.height = viewConfigs[i].recommendedImageRectHeight;
        sci.faceCount = 1;
        sci.arraySize = 1;
        sci.mipCount = 1;

        result = xrCreateSwapchain(session, &sci, swapchains->data() + i);
        if (!CheckResult(instance, result, "xrCreateSwapchain"))
        {
            return false;
        }

        result = xrEnumerateSwapchainImages((*swapchains)[i], 0, swapchainLengths.data() + i, NULL);
        if (!CheckResult(instance, result, "xrEnumerateSwapchainImages"))
        {
            return false;
        }
    }

    swapchainImages->resize(viewConfigs.size());
    for (uint32_t i = 0; i < viewConfigs.size(); i++)
    {
        (*swapchainImages)[i].resize(swapchainLengths[i]);
        for (uint32_t j = 0; j < swapchainLengths[i]; j++)
        {
            (*swapchainImages)[i][j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
            (*swapchainImages)[i][j].next = NULL;
        }

        result = xrEnumerateSwapchainImages((*swapchains)[i], swapchainLengths[i], &swapchainLengths[i],
                                            (XrSwapchainImageBaseHeader*)((*swapchainImages)[i].data()));
        if (!CheckResult(instance, result, "xrEnumerateSwapchainImages"))
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

bool CreateFrameBuffers(const std::vector<XrViewConfigurationView>& viewConfig, std::vector<GLuint>* frameBuffers, std::vector<GLuint>* depthBuffers)
{
    frameBuffers->resize(viewConfig.size(), 0);
    depthBuffers->resize(viewConfig.size(), 0);
    for (size_t i = 0; i < viewConfig.size(); i++)
    {
        glGenRenderbuffers(1, depthBuffers->data() + i);
        glBindRenderbuffer(GL_RENDERBUFFER, (*depthBuffers)[i]);
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
        glGenFramebuffers(1, frameBuffers->data() + i);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (*frameBuffers)[i]);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, (*depthBuffers)[i]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (!SupportsGL())
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

    std::vector<XrViewConfigurationView> viewConfigs;
    if (!EnumerateViews(instance, systemId, &viewConfigs))
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

    XrSession session;
    if (!CreateSession(instance, systemId, &session))
    {
        return 1;
    }

    XrActionSet actionSet = XR_NULL_HANDLE;
    if (!CreateActions(instance, systemId, session, &actionSet))
    {
        return 1;
    }

    XrSpace stageSpace;
    if (!CreateStageSpace(instance, systemId, session, &stageSpace))
    {
        return 1;
    }

    std::vector<XrSwapchain> swapchains;
    std::vector<std::vector<XrSwapchainImageOpenGLKHR>> swapchainImages;
    if (!CreateSwapchains(instance, systemId, session, viewConfigs, &swapchains, &swapchainImages))
    {
        return 1;
    }

    std::vector<GLuint> frameBuffers;
    std::vector<GLuint> depthBuffers;
    if (!CreateFrameBuffers(viewConfigs, &frameBuffers, &depthBuffers))
    {
        return 1;
    }

    // AJT: TODO: setup actions and actionsets for input.

    bool sessionReady = false;
    XrSessionState xrState = XR_SESSION_STATE_UNKNOWN;
    while (!quitting)
    {
        XrEventDataBuffer xrEvent;
        xrEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
        xrEvent.next = NULL;

        XrResult result = xrPollEvent(instance, &xrEvent);
        if (result == XR_SUCCESS)
        {
            switch (xrEvent.type)
            {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                printf("xrEvent: XR_TYPE_EVENT_DATA_EVENTS_LOST\n");
                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                printf("xrEvent: XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING\n");
                break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            {
                printf("xrEvent: XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED -> ");
                XrEventDataSessionStateChanged* ssc = (XrEventDataSessionStateChanged*)&xrEvent;
                xrState = ssc->state;
                switch (xrState)
                {
                case XR_SESSION_STATE_IDLE:
                    printf("XR_SESSION_STATE_IDLE\n");
                    break;
                case XR_SESSION_STATE_READY:
                    printf("XR_SESSION_STATE_READY\n");
                    if (!BeginSession(instance, systemId, session))
                    {
                        return 1;
                    }
                    sessionReady = true;
                    break;
                case XR_SESSION_STATE_SYNCHRONIZED:
                    printf("XR_SESSION_STATE_SYNCHRONIZED\n");
                    break;
                case XR_SESSION_STATE_VISIBLE:
                    printf("XR_SESSION_STATE_VISIBLE\n");
                    break;
                case XR_SESSION_STATE_FOCUSED:
                    printf("XR_SESSION_STATE_FOCUSED\n");
                    break;
                case XR_SESSION_STATE_STOPPING:
                    printf("XR_SESSION_STATE_STOPPING\n");
                    break;
                case XR_SESSION_STATE_LOSS_PENDING:
                    printf("XR_SESSION_STATE_LOSS_PENDING\n");
                    break;
                case XR_SESSION_STATE_EXITING:
                    printf("XR_SESSION_STATE_EXITING\n");
                    break;
                default:
                    printf("XR_SESSION_STATE_??? %d\n", (int)xrState);
                    break;
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                printf("XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING\n");
                break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                printf("XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED\n");
                break;
            case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR:
                printf("XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR\n");
                break;
            case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
                printf("XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT\n");
                break;
            default:
                printf("Unhandled event type %d\n", xrEvent.type);
                break;
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event) )
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

            // syncInput
            XrActiveActionSet aas;
            aas.actionSet = actionSet;
            aas.subactionPath = XR_NULL_PATH;
            XrActionsSyncInfo asi;
            asi.type = XR_TYPE_ACTIONS_SYNC_INFO;
            asi.next = NULL;
            asi.countActiveActionSets = 1;
            asi.activeActionSets = &aas;
            result = xrSyncActions(session, &asi);
            if (!CheckResult(instance, result, "xrSyncActions"))
            {
                return 1;
            }
            printf("after xrSyncActions\n");

            // AJT: TODO xrGetActionStateFloat()
            // xrGetActionStatePose()

            XrFrameState fs;
            fs.type = XR_TYPE_FRAME_STATE;
            fs.next = NULL;

            XrFrameWaitInfo fwi;
            fwi.type = XR_TYPE_FRAME_WAIT_INFO;
            fwi.next = NULL;

            result = xrWaitFrame(session, &fwi, &fs);
            if (!CheckResult(instance, result, "xrWaitFrame"))
            {
                return 1;
            }

            printf("after xrWaitFrame\n");

            XrFrameBeginInfo fbi;
            fbi.type = XR_TYPE_FRAME_BEGIN_INFO;
            fbi.next = NULL;
            result = xrBeginFrame(session, &fbi);
            if (!CheckResult(instance, result, "xrBeginFrame"))
            {
                return 1;
            }

            printf("after xrBeginFrame\n");

            // TODO: xrLocateViews
            // for each view
            // {
            //    xrAquireSwapcahinImage
            //    xrWaitSwapchainImage
            //    render
            //    xrReleaseSwapchainImage
            // }

            XrFrameEndInfo fei;
            fei.type = XR_TYPE_FRAME_END_INFO;
            fei.next = NULL;
            fei.displayTime = fs.predictedDisplayTime;
            fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            fei.layerCount = 0;//(uint32_t)layers.size();
            fei.layers = nullptr; //layers.data();
            result = xrEndFrame(session, &fei);
            if (!CheckResult(instance, result, "xrEndFrame"))
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

    for (auto& frameBuffer : frameBuffers)
    {
        glDeleteFramebuffers(1, &frameBuffer);
    }

    for (auto& depthBuffer : depthBuffers)
    {
        glDeleteRenderbuffers(1, &depthBuffer);
    }

    XrResult result;
    for (auto& swapchain : swapchains)
    {
        result = xrDestroySwapchain(swapchain);
        CheckResult(instance, result, "xrDestroySwapchain");
    }

    result = xrDestroySpace(stageSpace);
    CheckResult(instance, result, "xrDestroySpace");

    xrEndSession(session);
    CheckResult(instance, result, "xrEndSession");

    xrDestroySession(session);
    CheckResult(instance, result, "xrDestroySession");

    xrDestroyInstance(instance);
    CheckResult(XR_NULL_HANDLE, result, "xrDestroyInstance");

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
