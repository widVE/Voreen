//
//  FionaVoreen.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/4/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef VOREEN_FIONA_H__
#define VOREEN_FIONA_H__

#include "FionaScene.h"

#include <string>
#include <vector>

#define _WINDOWS
#define NOMINMAX
#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#define DLL_TEMPLATE_INST
#define TIXML_USE_STL
#define TGT_WITHOUT_DEFINES
#define TGT_DEBUG
//#define VRN_STATIC_LIBS
#define VRN_DEBUG
#define BOOST_ALL_NO_LIB
#define EIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS
#define VRN_MODULE_BASE
#define VRN_MODULE_CONNEXE
#define VRN_MODULE_CORE
#define VRN_MODULE_DEVIL
#define TGT_HAS_DEVIL
#define VRN_MODULE_DYNAMICGLSL
#define VRN_MODULE_FLOWREEN
#define VRN_MODULE_PLOTTING
#define VRN_MODULE_PVM
#define VRN_MODULE_SEGY
#define VRN_MODULE_STAGING
#define VRN_MODULE_VOLUMELABELING
#define VRN_MODULE_ZIP

#include "voreen/core/voreenapplication.h"
#include "voreen/core/properties/cameraproperty.h"
#include "tgt/init.h"
#include "modules/core/processors/output/canvasrenderer.h"
#include "modules/base/processors/proxygeometry/cubeproxygeometry.h"
#include "modules/base/processors/geometry/planewidgetprocessor.h"
#include "modules/base/processors/proxygeometry/optimizedproxygeometry.h"
#include "voreen/core/network/workspace.h"
#include "modules/staging/processors/arbitraryvolumeclipping.h"

#include "FionaNetwork.h"

class FionaVoreen : public FionaScene
{
public:
	FionaVoreen(const std::string& filename, int tw=512, int th=512, const std::string& root = "");
	virtual ~FionaVoreen();

	virtual void preRender(float fTime) { FionaScene::preRender(fTime); }
	virtual void render(void);
	virtual void buttons(int button, int state);
	virtual void updateJoystick(const jvec3& v);
	virtual void updateLeap(void);
	// If the systemwide voreen system initialization
	//  following variable should be a "static" variable,
	//  but, I want this wrapper to be a simple header file..
	//  so, I leave it as a member variable and add another in the following
	//  function.
	bool voreenInitialized;
	// Caution: this function should be called with GLContext enabled
	void initVoreen(const std::string& basePath);

	bool hasClipPlanes(void) const { return clipPlanes != 0; }
	int	getClipPlane(void) const;
	voreen::FloatProperty * getClipProperty(const char *sName);
	void setClipPlane(int clip) { clipPlanes->grabbed_ = clip; }
	void setClipProperty(const char *sName, bool bOn);
	void turnOnJoystickAction(bool bOn);

private:
	static std::vector<voreen::CameraProperty*> getCameraProperties(voreen::NetworkEvaluator* eval);
	static tgt::Camera getCamera(voreen::NetworkEvaluator* eval);
	static tgt::mat4 glGetMatrix(GLuint matrix);
	static void updateProjection(voreen::NetworkEvaluator* eval);
#ifndef LINUX_BUILD
	void drawHand(const LeapData::HandData &hand);
#endif
	int loadWorkspace(const std::string& filename, int tw=512, int th=512);
	
	voreen::VoreenApplication* app;
	voreen::CanvasRenderer* renderer;
	voreen::NetworkEvaluator* eval;
	voreen::CubeProxyGeometry *clipCube;
	voreen::PlaneWidgetProcessor *clipPlanes;
	voreen::Workspace* workspace;
	voreen::ArbitraryVolumeClipping *freeClip;

//	int viewportWidth, viewportHeight;
	int textureWidth, textureHeight;
	std::string voreenRoot;
	std::string workspaceFilename;
	int buttonBitArray;	//could also use fionaConf.currentButtons
	VRWandAction *m_joystickAction;

#if ENABLE_OCULUS
	GLuint m_quadVertexBuffer;
	GLuint m_quadLeftUVBuffer;
	GLuint m_quadRightUVBuffer;
	GLuint correctionProgram;
	GLuint uniformLensCenter;
	GLuint uniformScreenCenter;
	GLuint uniformScale;
	GLuint uniformScaleIn;
	GLuint uniformHMDWarpParam;
	GLuint uniformChromAbParam;
	GLuint uniformTexture0;

	static const char* PostProcessVertexShaderSrc;
	static const char* PostProcessFullFragShaderSrc;
	static const char* PostProcessFragShaderSrc;
	static const char* PostProcessFragTestShaderSrc;

	void createOculusCorrection(void);
#endif
};

#endif



