#include "FionaVoreen.h"

#include "FionaUT.h"

#include "VRVolumeAction.h"

#include "voreen/core/utils/voreenpainter.h"
#include "voreen/core/io/serialization/serialization.h"
#include "voreen/core/network/networkevaluator.h"
#include "voreen/core/network/processornetwork.h"

#include "modules/core/processors/output/canvasrenderer.h"
#include "voreen/core/properties/condition.h"
#include "voreen/core/ports/port.h"


#ifdef NDEBUG
bool myCustomAssert(long, char const *, char const *, char const *, bool &) { return false; }
#endif

#ifdef LINUX_BUILD
bool myCustomAssert(long, char const *, char const *, char const *, bool &) { return false; }
#endif

//#include "voreen/core/utils/commandlineparser.h"
const std::string voreen::Port::loggerCat_("voreen.Port");

#ifndef LINUX_BUILD
template<>
bool voreen::NumericPropertyValidation<float>::met() const throw() {
    // if min and max make sense do a real validation
    if (observed_->minValue_ <= observed_->maxValue_) {
        return ((observed_->value_ >= observed_->minValue_)
            && (observed_->value_ <= observed_->maxValue_));
    }
    else
        return true;
}

template<>
bool voreen::NumericPropertyValidation<int>::met() const throw() {
    // if min and max make sense do a real validation
    if (observed_->minValue_ <= observed_->maxValue_) {
        return ((observed_->value_ >= observed_->minValue_)
            && (observed_->value_ <= observed_->maxValue_));
    }
    else
        return true;
}

template<typename T>
bool voreen::NumericPropertyValidation<T>::met() const throw() {
    const T& min = observed_->minValue_;
    const T& max = observed_->maxValue_;

    // do min and max make sense at all?
    // If not just pretend the validation holds
    for (size_t i = 0; i < min.size; ++i) {
        if (min.elem[i] > max.elem[i])
            return true;
    }

    // If we have come this far we can do real validations:
    // test if any component is < min or > max
    const T& val = observed_->value_;
    for (size_t i = 0; i < val.size; ++i) {
        if ((val.elem[i] < min.elem[i]) || (val.elem[i] > max.elem[i]))
            return false;
    }
    return true;
}
#endif

template<class T>
std::string voreen::NumericPropertyValidation<T>::description() const{
    std::stringstream stream;
    stream << " for '" << observed_->getFullyQualifiedID() << "': "
           << observed_->value_ << " out of valid range [" << observed_->minValue_ << "," << observed_->maxValue_ << "]";
    return stream.str();
}


#ifdef ENABLE_OCULUS
#include <Kit3D/glslUtils.h>

const char* FionaVoreen::PostProcessVertexShaderSrc =
    "attribute vec3 Position;\n"
	"attribute vec2 TexCoord;\n"
    "varying vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(Position, 1.0);\n"
	"	oTexCoord = TexCoord;\n"
    "}\n";

const char* FionaVoreen::PostProcessFragShaderSrc =
    "uniform vec2 LensCenter;\n"
    "uniform vec2 ScreenCenter;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 ScaleIn;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform sampler2D Texture0;\n"
    "varying vec2 oTexCoord;\n"
    "\n"
    "vec2 HmdWarp(vec2 in01)\n"
    "{\n"
    "   vec2 theta = (in01 - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
    "   float rSq = theta.x * theta.x + theta.y * theta.y;\n"
    "   vec2 theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
	"   return LensCenter + (Scale * theta1);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "   vec2 tc = HmdWarp(oTexCoord);\n"
    "   if (!all(equal(clamp(tc, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tc)))\n"
    "       gl_FragColor = vec4(0);\n"
    "   else\n"
    "       gl_FragColor = texture2D(Texture0, tc);\n"
    "}\n";

const char* FionaVoreen::PostProcessFragTestShaderSrc =
    "uniform sampler2D Texture0;\n"
    "varying vec2 oTexCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
	"   gl_FragColor = texture2D(Texture0, oTexCoord);\n"
    "}\n";

// Shader with lens distortion and chromatic aberration correction.
const char* FionaVoreen::PostProcessFullFragShaderSrc =
    "uniform vec2 LensCenter;\n"
    "uniform vec2 ScreenCenter;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 ScaleIn;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform vec4 ChromAbParam;\n"
    "uniform sampler2D Texture0;\n"
    "varying vec2 oTexCoord;\n"
    "\n"
    // Scales input texture coordinates for distortion.
    // ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
    // larger due to aspect ratio.
    "void main()\n"
    "{\n"
    "   vec2  theta = (oTexCoord - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
    "   float rSq= theta.x * theta.x + theta.y * theta.y;\n"
    "   vec2  theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + "
    "                  HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
    "   \n"
    "   // Detect whether blue texture coordinates are out of range since these will scaled out the furthest.\n"
    "   vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);\n"
    "   vec2 tcBlue = LensCenter + Scale * thetaBlue;\n"
    "   if (!all(equal(clamp(tcBlue, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tcBlue)))\n"
    "   {\n"
    "       gl_FragColor = vec4(0);\n"
    "       return;\n"
    "   }\n"
    "   \n"
    "   // Now do blue texture lookup.\n"
    "   float blue = texture2D(Texture0, tcBlue).b;\n"
    "   \n"
    "   // Do green lookup (no scaling).\n"
    "   vec2  tcGreen = LensCenter + Scale * theta1;\n"
    "   vec4  center = texture2D(Texture0, tcGreen);\n"
    "   \n"
    "   // Do red scale and lookup.\n"
    "   vec2  thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);\n"
    "   vec2  tcRed = LensCenter + Scale * thetaRed;\n"
    "   float red = texture2D(Texture0, tcRed).r;\n"
    "   \n"
    "   gl_FragColor = vec4(red, center.g, blue, center.a);\n"
    "}\n";

void FionaVoreen::createOculusCorrection(void)
{
	correctionProgram = loadProgram(std::string(PostProcessVertexShaderSrc), std::string(PostProcessFullFragShaderSrc), false);
	
	uniformLensCenter=glGetUniformLocation(correctionProgram, "LensCenter");
	uniformScreenCenter=glGetUniformLocation(correctionProgram, "ScreenCenter");
	uniformScale=glGetUniformLocation(correctionProgram, "Scale");
	uniformScaleIn=glGetUniformLocation(correctionProgram, "ScaleIn");
	uniformHMDWarpParam=glGetUniformLocation(correctionProgram, "HmdWarpParam");
	uniformChromAbParam=glGetUniformLocation(correctionProgram, "ChromAbParam");
	uniformTexture0=glGetUniformLocation(correctionProgram, "Texture0");
}
#endif

FionaVoreen::FionaVoreen(const std::string& filename, int tw, int th, const std::string& root)
: voreenInitialized(false), app(0), renderer(0), eval(0), workspace(0), clipCube(0), clipPlanes(0), textureWidth(tw), textureHeight(th), 
	voreenRoot(root), workspaceFilename(filename), buttonBitArray(0), m_joystickAction(0), freeClip(0)
{
	if( root.length()<=0 ) 
		voreenRoot = fionaConf.VoreenRoot;

	//this makes us use external frame buffer object (slightly faster rendering as we don't have to re-draw the quad ourselves)	
	_FionaUTUseExtFBO(true);

#ifdef ENABLE_OCULUS
	m_quadVertexBuffer = 0;
	m_quadLeftUVBuffer = 0;
	m_quadRightUVBuffer = 0;
	correctionProgram=0;
	uniformLensCenter=0;
	uniformScreenCenter=0;
	uniformScale=0;
	uniformScaleIn=0;
	uniformHMDWarpParam=0;
	uniformChromAbParam=0;
	uniformTexture0=0;
#endif
}

FionaVoreen::~FionaVoreen()
{
	if(eval)
	{
		delete eval;
		eval = 0;
	}

    if (app) 
	{
		app->deinitializeGL();
		app->deinitialize();
		/*if(workspace)
		{
			workspace->clear();
		}*/  
    }

	//delete workspace;
	//workspace = 0;
	delete app;
	app = 0;

#ifdef ENABLE_OCULUS
	glDeleteBuffers(1, &m_quadVertexBuffer);
	glDeleteBuffers(1, &m_quadLeftUVBuffer);
	glDeleteBuffers(1, &m_quadRightUVBuffer);
	glDeleteProgram(correctionProgram);
#endif
}

int	FionaVoreen::getClipPlane(void) const
{
	if(clipPlanes)
	{
		return clipPlanes->grabbed_;
	}
	return -1;
}

// If the systemwide voreen system initialization
//  following variable should be a "static" variable,
//  but, I want this wrapper to be a simple header file..
//  so, I leave it as a member variable and add another in the following
//  function. static bool inited = false;
// Caution: this function should be called with GLContext enabled
void FionaVoreen::initVoreen(const std::string& basePath)
{
	printf("Initialize Voreen...\n");
	static bool inited = false;
	if( inited ) return;
	char* argv[]={(char*)basePath.c_str()};	//need to pass basepath into args for voreen 4.0.1 otherwise it uses current program directory...
#ifdef LINUX_BUILD	
	char hostname[1024];
	hostname[1023] = '\n';
	gethostname(hostname, 1023);
	app = new voreen::VoreenApplication("FionaVoreen4", hostname, hostname,1, argv,voreen::VoreenApplication::APP_ALL);
	//printf("Voreen temp directory:%s\n", app->getTemporaryPath().c_str());
	app->setLoggingEnabled(false);
	//app->setLogFile(std::string(""));
#else
	char sMachine[32];
	memset(sMachine, 0, sizeof(sMachine));
	sprintf(sMachine, "%u%u", fionaConf.appType, fionaConf.kevinOffset.x < 0.f ? 0 : 1);
	std::string strMachine(sMachine);
	app = new voreen::VoreenApplication(std::string("FionaVoreen")+strMachine, strMachine, strMachine, 1, argv, voreen::VoreenApplication::APP_ALL);
	
#endif
	app->initialize();
	app->initializeGL();

	inited = true;
	printf("Initialized ...\n");

#ifdef ENABLE_OCULUS
	createOculusCorrection();
#endif
}

//***************************************
//
// Workspace related functions
//
//**************************************
int FionaVoreen::loadWorkspace(const std::string& filename, int tw, int th)
{
	workspace = new voreen::Workspace();
	try {
		workspace->load(filename);
	}
	catch (voreen::SerializationException& e) {
		LERRORC("FionaVoreen.initialize", "Failed to load standard workspace: " << e.what());
		return false;
		//		exit(EXIT_FAILURE);
	}
		
	// initialize the network evaluator and retrieve CanvasRenderer processors from the loaded network
	eval = new voreen::NetworkEvaluator();
	voreen::ProcessorNetwork* network = workspace->getProcessorNetwork();
	eval->setProcessorNetwork(network);
	std::vector<voreen::CanvasRenderer*> canvasRenderers = network->getProcessorsByType<voreen::CanvasRenderer>();
	if (canvasRenderers.empty()) {
		LERRORC("FionaVoreen.initialize", "Loaded standard workspace does not contain a CanvasRenderer");
		return false;
		//		exit(EXIT_FAILURE);
	}
	renderer = canvasRenderers[0];

	std::vector<voreen::CubeProxyGeometry*> cubeProxies = network->getProcessorsByType<voreen::CubeProxyGeometry>();
	if(!cubeProxies.empty())
	{
		clipCube = cubeProxies[0];
	}

	std::vector<voreen::PlaneWidgetProcessor*> planeProcessors = network->getProcessorsByType<voreen::PlaneWidgetProcessor>();
	if(!planeProcessors.empty())
	{
		clipPlanes = planeProcessors[0];
		clipPlanes->grabbed_ = -1;
		voreen::BoolProperty *showInner = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("showInnerBB"));
		showInner->set(true);
		voreen::FloatVec4Property *innerColor = dynamic_cast<voreen::FloatVec4Property*>(clipPlanes->getProperty("innerColor"));
		innerColor->set(tgt::vec4(1.f, 0.f, 1.f, 1.f));
		//disable all manipulators until user enables them w/ wand..
		voreen::BoolProperty *leftX = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("enableLeftX"));
		voreen::BoolProperty *rightX = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("enableRightX"));
		voreen::BoolProperty *bottomY = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("enableBottomY"));
		voreen::BoolProperty *topY = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("enableTopY"));
		voreen::BoolProperty *frontZ = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("enableFrontZ"));
		voreen::BoolProperty *backZ = dynamic_cast<voreen::BoolProperty*>(clipPlanes->getProperty("enableBackZ"));
		leftX->set(false);
		rightX->set(false);
		bottomY->set(false);
		topY->set(false);
		frontZ->set(false);
		backZ->set(false);
	}

	std::vector<voreen::ArbitraryVolumeClipping*> clipProxies = network->getProcessorsByType<voreen::ArbitraryVolumeClipping>();
	if(!clipProxies.empty())
	{
		freeClip = clipProxies[0];
	}

	//setup actions here..
	VRActionSet *pVoreenActions = new VRActionSet();
	pVoreenActions->SetName(std::string("voreen"));

	VRClip *pClip = new VRClip();
	pClip->SetButton(5);
	pClip->SetOnRelease(true);
	pClip->SetScenePtr(this);
	pVoreenActions->AddAction(pClip);

	VRToggleInnerBox *pInner = new VRToggleInnerBox();
	pInner->SetButton(2);
	pInner->SetOnRelease(true);
	pInner->SetScenePtr(this);
	pVoreenActions->AddAction(pInner);

	VRResetClip *pResetClip = new VRResetClip();
	pResetClip->SetButton(3);
	pResetClip->SetOnRelease(true);
	pResetClip->SetScenePtr(this);
	pVoreenActions->AddAction(pResetClip);

	m_joystickAction = new VRAdjustClip();
	m_joystickAction->SetScenePtr(this);

	//pVoreenActions->SetJoystickAction(m_joystickAction);
	m_actions.AddSet(pVoreenActions);
	m_actions.SetCurrentSet(pVoreenActions);

	//todo allow the size of the actual rendering to be command-line params..
	std::vector<voreen::Port*> ports = renderer->getInports();
	voreen::RenderPort* port = (voreen::RenderPort*)ports[0];
	port->resize(tgt::ivec2(tw,th));

	return true;
}

void FionaVoreen::render(void)
{
	FionaScene::render();

	glClearColor(0,0,0,0);
	if(!voreenInitialized)
	{
		initVoreen(voreenRoot);
		voreenInitialized = true;
	}
	if(voreenInitialized && eval==NULL)
	{
		printf("trying to load voreen workspace %s\n", workspaceFilename.c_str());
		if(!loadWorkspace(workspaceFilename,textureWidth,textureHeight))
		{
			if(eval!=NULL) delete eval;
			eval = NULL;
		}
		else
		{
			printf("loaded voreen workspace!\n");
		}
	}
	if( eval==NULL || !voreenInitialized ) return;
		
	/*const voreen::ProcessorNetwork* network = eval->getProcessorNetwork();
	const std::vector<voreen::Processor*> processors = network->getProcessors();
	for( int i=0; i<(int)processors.size(); i++ )
	{
		std::vector<voreen::CameraProperty*> props
		= processors[i]->getPropertiesByType<voreen::CameraProperty>();
				if(!props.empty())
					processors[i]->invalidate();
	}*/

	updateProjection(eval);

	glPushAttrib(GL_VIEWPORT_BIT);
	glMatrixMode(GL_PROJECTION); 
	glPushMatrix(); 
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); 
	glPushMatrix(); 
	glLoadIdentity();

	eval->process();

	glMatrixMode(GL_MODELVIEW); 
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();

	tgt::Texture* texture = renderer->getImageColorTexture();
	if( texture==NULL) printf("texture is NULL\n");

#ifdef FIONA_UT_H__
	if( _FionaUTIsInFBO() )
	{
		//current rendering goes in here when SetExtFBO(true) is commented in in constructor...
		//printf("Using external FBO...\n");
		_FionaUTSetExtFBOTexture(texture->getId());
		return;
	}
#endif
	//printf("render\n");
	glPushAttrib(GL_TEXTURE_BIT|GL_LIGHTING_BIT
					|GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
		
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glColor4f(1,1,1,1);
	glEnable(GL_TEXTURE_2D);	
		
	glBindTexture(GL_TEXTURE_2D, texture->getId());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		
#ifdef ENABLE_OCULUS
	int params[4] = {0};
	glGetIntegerv( GL_VIEWPORT, (GLint*)params );
	const GLint vpx = params[0];
	const GLint vpy = params[1];
	const GLsizei vpwidth = params[2];
	const GLsizei vpheight = params[3];
	const GLsizei h = vpheight;
	const GLsizei w = vpwidth*2;

#ifdef ENABLE_DK2
	   
	float shaderw = float(vpwidth) / float(w);
	float shaderh = float(vpheight) / float(h);
	float shaderx = float(vpx) / float(w);
	float shadery = float(vpy) / float(h);
	float as = float(vpwidth) / float(vpheight);

	//float XCenterOffset = 0.0379941;//d.XCenterOffset;
	//if( eye == 2 )
	//	XCenterOffset = -0.0379941;//d.XCenterOffset;
	//d.LensCenter.x = -0.00986
	//d.LensCenter.x right = 0.00986
	//double check "eye" replacement with fionaRenderCycleLeft below...
	float lensCenterx = fionaRenderCycleLeft ? 0.25 + 0.00986 : 0.75 - 0.00986;//shaderx + (shaderw + XCenterOffset * 0.5f)*0.5f;//d.LensCenter.x;
	float lensCentery = shadery + shaderh*0.5f;//d.LensCenter.y;

	//printf("Old: %f, %f; New: %f, %f\n", lensCenterx, lensCentery, d.LensCenter.x, d.LensCenter.y);

	float scaleFactor = 1.0f / 1.3527f;//1.71f;// / d.Scale;

	glUseProgram(correctionProgram);
	glUniform2f(uniformLensCenter, lensCenterx, lensCentery);
	glUniform2f(uniformScreenCenter, shaderx + shaderw*0.5f, shadery + shaderh*0.5f);
	glUniform2f(uniformScale, (shaderw/2.f) * scaleFactor, (shaderh/2.f) * scaleFactor * as);
	glUniform2f(uniformScaleIn, 2.f/shaderw, (2.f/shaderh)/as);
	glUniform4f(uniformHMDWarpParam, 1.f, 0.22f, 0.24f, 0.0f);//d.K[0], d.K[1], d.K[2], d.K[3]);
	glUniform4f(uniformChromAbParam, 1.f, 0.f, 1.f, 0.f);//d.ChromaticAberration[0], d.ChromaticAberration[1], d.ChromaticAberration[2], d.ChromaticAberration[3]);
	glUniform1i(uniformTexture0, 0);
#else


	const OVR::Util::Render::DistortionConfig d = fionaConf.stereoConfig.GetDistortionConfig();
	
	float scaleFactor = 1.0f / d.Scale;
	float shaderw = float(vpwidth) / float(w);
	float shaderh = float(vpheight) / float(h);
	float shaderx = float(vpx) / float(w);
	float shadery = float(vpy) / float(h);
	float as = float(vpwidth) / float(vpheight);
	float lensCenterx = shaderx + (shaderw + d.XCenterOffset * 0.5f)*0.5f;
	float lensCentery = shadery + shaderh*0.5f;
	float screenCenterx = shaderx + shaderw*0.5f;
	float screenCentery = shadery + shaderh*0.5f;

	float scalex = (shaderw/2.f) * scaleFactor;
	float scaley = (shaderh/2.f) * scaleFactor * as;
	float scaleinx = 2.f/shaderw;
	float scaleiny = (2.f/shaderh)/as;

	glUseProgram(correctionProgram);
	glUniform2f(uniformLensCenter, lensCenterx, lensCentery);
	glUniform2f(uniformScreenCenter, screenCenterx, screenCentery);
	glUniform2f(uniformScale, scalex, scaley);
	glUniform2f(uniformScaleIn, scaleinx, scaleiny);
	glUniform4f(uniformHMDWarpParam, d.K[0], d.K[1], d.K[2], d.K[3]);
	glUniform4f(uniformChromAbParam, d.ChromaticAberration[0], d.ChromaticAberration[1], d.ChromaticAberration[2], d.ChromaticAberration[3]);
	glUniform1i(uniformTexture0, 0);
#endif

	static const GLfloat g_quad_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			1.0f,  1.0f, 0.0f,
	};

	static const GLfloat q_tex_buffer_data_left[] = {
		0.f, 0.f,
		0.5f, 0.f,
		0.f, 1.f,
		0.f, 1.f,
		0.5f, 0.f,
		0.5f, 1.f };

	static const GLfloat q_tex_buffer_data_right[] = {
		0.5f, 0.f,
		1.f, 0.f,
		0.5f, 1.f,
		0.5f, 1.f,
		1.f, 0.f,
		1.f, 1.f };

	if(m_quadVertexBuffer == 0)
	{
		glGenBuffers(1, &m_quadVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	}

	if(m_quadLeftUVBuffer == 0)
	{
		glGenBuffers(1, &m_quadLeftUVBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadLeftUVBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(q_tex_buffer_data_left), q_tex_buffer_data_left, GL_STATIC_DRAW);
	}
	
	if(m_quadRightUVBuffer == 0)
	{
		glGenBuffers(1, &m_quadRightUVBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_quadRightUVBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(q_tex_buffer_data_left), q_tex_buffer_data_right, GL_STATIC_DRAW);
	}

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVertexBuffer);
  glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
  );

  if(fionaRenderCycleLeft)
  {
	  glEnableVertexAttribArray(1);
	  glBindBuffer(GL_ARRAY_BUFFER, m_quadLeftUVBuffer);
	  glVertexAttribPointer(
			1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
	  );
  }
  else if(fionaRenderCycleRight)
  {
	  glEnableVertexAttribArray(1);
	  glBindBuffer(GL_ARRAY_BUFFER, m_quadRightUVBuffer);
	  glVertexAttribPointer(
			1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
	  );
  }

	// Draw the triangles !
  glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
#else
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f); glVertex2f(-1.f, -1.f);
	glTexCoord2f(1.f, 0.f); glVertex2f(1.f, -1.f);
	glTexCoord2f(1.f, 1.f); glVertex2f(1.f, 1.f);
	glTexCoord2f(0.f, 1.f); glVertex2f(-1.f, 1.f);
	glEnd();
#endif

#ifdef ENABLE_OCULUS
	glUseProgram(0);
#endif

	/*if(fionaConf.leapData.hand1.valid)
	{
		drawHand(fionaConf.leapData.hand1);
	}

	if(fionaConf.leapData.hand2.valid)
	{
		drawHand(fionaConf.leapData.hand2);
	}*/

	glMatrixMode(GL_MODELVIEW);		glPopMatrix();
	glMatrixMode(GL_PROJECTION);	glPopMatrix();
	glPopAttrib();
}

#ifndef LINUX_BUILD
void FionaVoreen::drawHand(const LeapData::HandData &hand)
{
	glLineWidth(5.f);
	glColor4f(1.f, 0.f, 1.f, 1.f);
	glBegin(GL_LINES);
	jvec3 vTracker;
	getTrackerWorldSpace(vTracker);
	vTracker.z -= 1.f;	//into screen 1 meter
	vTracker.y -= 0.5f;	//down 1 meter
	for(int i = 0; i < 5; ++i)
	{
		if(hand.fingers[i].valid)
		{
			float fLen = hand.fingers[i].length;
			glVertex3f(vTracker.x + hand.fingers[i].tipPosition[0], vTracker.y + hand.fingers[i].tipPosition[1], vTracker.z + hand.fingers[i].tipPosition[2]);
			glVertex3f(vTracker.x + (hand.fingers[i].tipPosition[0] + hand.fingers[i].tipDirection[0]*fLen), 
						vTracker.y + (hand.fingers[i].tipPosition[1] + hand.fingers[i].tipDirection[1]*fLen), 
						vTracker.z + (hand.fingers[i].tipPosition[2] + hand.fingers[i].tipDirection[2]*fLen));
		}
	}
	glEnd();
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glLineWidth(1.f);
}
#endif

void FionaVoreen::buttons(int button, int state)
{
	FionaScene::buttons(button, state);
	printf("button %d %s\n", button, (state==0)?"up":"down");
	state == 1 ? buttonBitArray |=(1<<button) : buttonBitArray &= ~(1<<button);

	/*if(button == 5 && state == 1 && clipPlanes != 0)
	{
		static const char* names[12] = {"enableLeftX", "", "enableRightX", "", "enableBottomY", "", "enableTopY", "", "enableBackZ", "", "enableFrontZ", ""};

		//switch amongst which plane is the current clip plane
		//render the outline as we change amongst the planes..
		int before = clipPlanes->grabbed_;

		if(before != -1)
		{
			voreen::TemplateProperty<bool> *p1 = dynamic_cast<voreen::TemplateProperty<bool>*>(clipPlanes->getProperty(names[before]));
			p1->set(false);
			//p1->invalidate();
		}
		else
		{
			voreen::TemplateProperty<bool> *showInner = dynamic_cast<voreen::TemplateProperty<bool>*>(clipPlanes->getProperty("showInnerBB"));
			showInner->set(true);
			//showInner->invalidate();
			voreen::Property *pProp = clipPlanes->getProperty(std::string("manipulatorScaling"));
			voreen::FloatProperty *pFProp = dynamic_cast<voreen::FloatProperty*>(pProp);
			if(pFProp)
			{
				pFProp->set(10.f);
			}
		}

		if(clipPlanes->grabbed_ == -1)
		{
			clipPlanes->grabbed_ = 0;
		}
		else
		{
			clipPlanes->grabbed_ += 2;
		}

		if(clipPlanes->grabbed_ > 10)
		{
			clipPlanes->grabbed_ = -1;
		}
		
		int after = clipPlanes->grabbed_;

		if(after != -1)
		{
			voreen::TemplateProperty<bool> *p1 = dynamic_cast<voreen::TemplateProperty<bool>*>(clipPlanes->getProperty(names[after]));
			p1->set(true);
			//p1->invalidate();
		}
		else
		{
			voreen::TemplateProperty<bool> *showInner = dynamic_cast<voreen::TemplateProperty<bool>*>(clipPlanes->getProperty("showInnerBB"));
			showInner->set(false);
			//showInner->invalidate();
		}

		//clipPlanes->invalidate();
	}*/
}

void FionaVoreen::updateLeap(void)
{
	if(freeClip != 0)
	{
		//set arbitrary clipping plane to leap hand plane if a hand is present..
		voreen::Property *pPlaneNorm = freeClip->getProperty("planeNormal");
		voreen::Property *pPlanePos = freeClip->getProperty("planePosition");//freeClip->getProperty("depth");

		voreen::FloatVec3Property * pPlane = dynamic_cast<voreen::FloatVec3Property*>(pPlaneNorm);
		voreen::FloatProperty *pPos = dynamic_cast<voreen::FloatProperty*>(pPlanePos);

		/*if(fionaConf.leapData.hand1.t != Leap::Gesture::Type::TYPE_INVALID)
		{
			printf("Gesture type: %d, state: %d\n", (int)fionaConf.leapData.hand1.t, (int)fionaConf.leapData.hand1.s);
		}*/

		if(pPlane && pPos)
		{
#ifndef LINUX_BUILD
			if(fionaConf.leapData.hand1.valid)
			{
				static const jvec3 vCenter(0.f, 0.35f, 0.f);
				jvec3 vHandDir(fionaConf.leapData.hand1.palmNormal[0], fionaConf.leapData.hand1.palmNormal[1], fionaConf.leapData.hand1.palmNormal[2]);
				vHandDir = vHandDir.normalize();
				//convert to voreen's point of view..
				jvec3 vHandPos(fionaConf.leapData.hand1.handPosition[0], fionaConf.leapData.hand1.handPosition[1], fionaConf.leapData.hand1.handPosition[2]);
				jvec3 vDist = vHandPos - vCenter;
				float fDist = vDist.len();
				//printf("%f, %f, %f\n", fionaConf.leapData.hand1.handPosition[0], fionaConf.leapData.hand1.handPosition[1], fionaConf.leapData.hand1.handPosition[2]);
				//printf("Dist from center: %f\n", fDist);
				pPos->set(-fDist*2.f);
				//pPos->set((1.f-fDist)*50.f);
				pPlane->set(tgt::vec3(-vHandDir.x, -vHandDir.y, -vHandDir.z));
			}
			else
			{
				pPos->set(0.f);
			}
#endif
		}
	}
}

void FionaVoreen::turnOnJoystickAction(bool bOn)
{
	if(bOn)
	{
		m_actions.GetCurrentSet()->SetJoystickAction(m_joystickAction);
	}
	else
	{
		m_actions.GetCurrentSet()->SetJoystickAction(0);
	}
}

void FionaVoreen::updateJoystick(const jvec3& v)
{
	if(clipPlanes)
	{
		if(clipPlanes->grabbed_ != -1 && (v.x != 0.f || v.z != 0.f))
		{
			//voreen::FloatProperty *width = dynamic_cast<voreen::FloatProperty*>(clipPlanes->getProperty("lineWidth"));
			//width->set(4.f);
			//printf("width: %f\n", width->get());
			//printf("grabbed: %d\n", clipPlanes->grabbed_);
			voreen::FloatProperty *pClip = 0;
			if(clipPlanes->grabbed_ == 0)
			{
				//left
				voreen::Property *pProp = clipPlanes->getProperty("leftClipPlane");
				pClip = dynamic_cast<voreen::FloatProperty*>(pProp);
			}
			else if(clipPlanes->grabbed_ == 2)
			{
				//right
				pClip = dynamic_cast<voreen::FloatProperty*>(clipPlanes->getProperty("rightClipPlane"));
			}
			else if(clipPlanes->grabbed_ == 4)
			{
				//front
				pClip = dynamic_cast<voreen::FloatProperty*>(clipPlanes->getProperty("bottomClipPlane"));
			}
			else if(clipPlanes->grabbed_ == 6)
			{
				//back
				pClip = dynamic_cast<voreen::FloatProperty*>(clipPlanes->getProperty("topClipPlane"));
			}
			else if(clipPlanes->grabbed_ == 8)
			{
				//bottom
				pClip = dynamic_cast<voreen::FloatProperty*>(clipPlanes->getProperty("frontClipPlane"));
			}
			else if(clipPlanes->grabbed_ == 10)
			{
				//top
				pClip = dynamic_cast<voreen::FloatProperty*>(clipPlanes->getProperty("backClipPlane"));
			}

			if(pClip)
			{
				//pClip->toggleInteractionMode(true, pClip);
				float fClip = pClip->get();
				//printf("clip: %f\n", fClip);
				//printf("joystick: %f\n", v.x);
				if(clipPlanes->grabbed_ == 4 || clipPlanes->grabbed_ == 6)
				{
					fClip += v.z;
				}
				else
				{
					fClip += v.x;
				}

				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}

			//clipPlanes->invalidate();
			return;
		}
	}

	/*if(clipCube != 0)
	{
		if(buttonBitArray == 0)
		{
			FionaScene::updateJoystick(v);
			return;
		}

		voreen::FloatProperty *pClip = 0;

		if(buttonBitArray & 1)
		{
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("backClippingPlane"));
			if(pClip)
			{
				float fClip = pClip->get();
				if(v.z > 0.f)
				{
					fClip += 5.f;
				}
				else if(v.z < 0.f)
				{
					fClip -= 5.f;
				}

				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}
		}
		
		if(buttonBitArray & 2)
		{
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("frontClippingPlane"));
			if(pClip)
			{
				float fClip = pClip->get();
				if(v.z < 0.f)
				{
					fClip -= 5.f;
				}
				else if(v.z > 0.f)
				{
					fClip += 5.f;
				}
				
				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}
		}
		
		if(buttonBitArray & 4)
		{
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("leftClippingPlane"));
			if(pClip)
			{
				float fClip = pClip->get();
				if(v.x > 0.f)
				{
					fClip -= 5.f;
				}
				else if(v.x < 0.f)
				{
					fClip += 5.f;
				}

				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}

			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("bottomClippingPlane"));
			if(pClip)
			{
				float fClip = pClip->get();
				if(v.z < 0.f)
				{
					fClip += 5.f;
				}
				else if(v.z > 0.f)
				{
					fClip -= 5.f;
				}
				
				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}
		}
		
		if(buttonBitArray & 8)
		{
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("rightClippingPlane"));
			if(pClip)
			{
				float fClip = pClip->get();
				if(v.x > 0.f)
				{
					fClip -= 5.f;
				}
				else if(v.x < 0.f)
				{
					fClip += 5.f;
				}
				
				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}

			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("topClippingPlane"));
			if(pClip)
			{
				float fClip = pClip->get();
				if(v.z < 0.f)
				{
					fClip += 5.f;
				}
				else if(v.z > 0.f)
				{
					fClip -= 5.f;
				}
				
				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}
		}
		
		if(buttonBitArray & 16)
		{

		}

		if(buttonBitArray & 32)
		{
			//reset all clip planes..
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("rightClippingPlane"));
			if(pClip)
			{
				pClip->set(pClip->getMinValue());
			}
			
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("leftClippingPlane"));
			if(pClip)
			{
				pClip->set(pClip->getMaxValue());
			}

			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("frontClippingPlane"));
			if(pClip)
			{
				pClip->set(pClip->getMinValue());
			}
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("backClippingPlane"));
			if(pClip)
			{
				pClip->set(pClip->getMaxValue());
			}

			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("bottomClippingPlane"));
			if(pClip)
			{
				pClip->set(pClip->getMinValue());
			}
			pClip = dynamic_cast<voreen::FloatProperty*>(clipCube->getProperty("topClippingPlane"));
			if(pClip)
			{
				pClip->set(pClip->getMaxValue());			
			}
		}
	}
	else
	{*/
		FionaScene::updateJoystick(v);
	//}
}

void FionaVoreen::setClipProperty(const char *sName, bool bOn)
{
	if(clipPlanes != 0)
	{
		voreen::Property *p = clipPlanes->getProperty(sName);
		voreen::TemplateProperty<bool> *pB = dynamic_cast<voreen::TemplateProperty<bool> *>(p);
		if(pB)
		{
			pB->set(bOn);
			pB->invalidate();
		}
	}
}

voreen::FloatProperty * FionaVoreen::getClipProperty(const char *sName)
{
	if(clipPlanes != 0)
	{
		voreen::Property *p = clipPlanes->getProperty(sName);
		voreen::FloatProperty *pF = dynamic_cast<voreen::FloatProperty*>(p);
		if(pF)
		{
			return pF;
		}
	}

	return 0;
}

//***************************************
//
//	static utility functions
//
//***************************************
	
std::vector<voreen::CameraProperty*> FionaVoreen::getCameraProperties(voreen::NetworkEvaluator* eval)
{
	std::vector<voreen::CameraProperty*> ret;
		
	const voreen::ProcessorNetwork* network = eval->getProcessorNetwork();
	const std::vector<voreen::Processor*> processors = network->getProcessors();
	for( int i=0; i<(int)processors.size(); i++ )
	{
		std::vector<voreen::CameraProperty*> props
		= processors[i]->getPropertiesByType<voreen::CameraProperty>();
		for( int j=0; j<(int)props.size(); j++ )	ret.push_back(props[j]);
	}
	return ret;
}
	
tgt::Camera FionaVoreen::getCamera(voreen::NetworkEvaluator* eval)
{
	const voreen::ProcessorNetwork* network = eval->getProcessorNetwork();
	const std::vector<voreen::Processor*> processors = network->getProcessors();
	for( int i=0; i<(int)processors.size(); i++ )
	{
		std::vector<voreen::CameraProperty*> props
		= processors[i]->getPropertiesByType<voreen::CameraProperty>();
		if( props.size()>0 ) return props[0]->get();
	}
	return tgt::Camera();
}
	
tgt::mat4 FionaVoreen::glGetMatrix(GLuint matrix)
{
	float m[16];
	glGetFloatv(matrix, m);
	return tgt::mat4(m[0],m[4],m[8],m[12],m[1],m[5],m[9],m[13],
						m[2],m[6],m[10],m[14],m[3],m[7],m[11],m[15]);
}
	
void FionaVoreen::updateProjection(voreen::NetworkEvaluator* eval)
{
	const voreen::ProcessorNetwork* network = eval->getProcessorNetwork();
	const std::vector<voreen::Processor*> processors = network->getProcessors();
	for( int i=0; i<(int)processors.size(); i++ )
	{
		std::vector<voreen::CameraProperty*> props
		= processors[i]->getPropertiesByType<voreen::CameraProperty>();
		for( int j=0; j<(int)props.size(); j++ )
		{
			tgt::Camera cam = props[j]->get();
			cam.setProjectionMode(tgt::Camera::MATRIX);
			cam.setProjectionMatrix(glGetMatrix(GL_PROJECTION_MATRIX));
			cam.setViewMatrix(glGetMatrix(GL_MODELVIEW_MATRIX));
			cam.setNearDist(fionaConf.nearClip);
			cam.setFarDist(fionaConf.farClip);
			cam.updateFrustum();
			//printf("%f\n", cam.getFarDist());
			props[j]->set(cam);
			//tgt::mat4 proj = cam.getProjectionMatrix();
		}
	}
}
