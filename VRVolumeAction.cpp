#include "VRVolumeAction.h"

#include "FionaVoreen.h"
#include "FionaUT.h"

void VRAdjustClip::JoystickMove(void)
{
	//set this as the action manager's joystick action..
	FionaVoreen *fionaScene = static_cast<FionaVoreen*>(m_scene);
	//get which plane is currently active (if any)
	if(fionaScene->getClipPlane() != -1)
	{
		if(fionaConf.currentJoystick.x != 0.f || fionaConf.currentJoystick.z != 0.f)
		{
			voreen::FloatProperty *pClip = 0;
			int grabbed = fionaScene->getClipPlane();

			if(grabbed == 0)
			{
				//left
				pClip = fionaScene->getClipProperty("leftClipPlane");
			}
			else if(grabbed == 2)
			{
				//right
				pClip = fionaScene->getClipProperty("rightClipPlane");
			}
			else if(grabbed == 4)
			{
				//front
				pClip = fionaScene->getClipProperty("bottomClipPlane");
			}
			else if(grabbed == 6)
			{
				//back
				pClip = fionaScene->getClipProperty("topClipPlane");
			}
			else if(grabbed == 8)
			{
				//bottom
				pClip = fionaScene->getClipProperty("frontClipPlane");
			}
			else if(grabbed == 10)
			{
				//top
				pClip = fionaScene->getClipProperty("backClipPlane");
			}

			if(pClip)
			{
				//pClip->toggleInteractionMode(true, pClip);
				float fClip = pClip->get();
				//printf("clip: %f\n", fClip);
				//printf("joystick: %f\n", v.x);
				if(grabbed == 0 || grabbed == 2)
				{
					fClip -= fionaConf.currentJoystick.x;
				}
				else if(grabbed == 4 || grabbed == 6)	//
				{
					fClip += fionaConf.currentJoystick.z;
				}
				else
				{
					fClip -= fionaConf.currentJoystick.z;
				}

				if(fClip > pClip->getMinValue() && fClip < pClip->getMaxValue())
				{
					pClip->set(fClip);
				}
			}
		}
	}
	/*else if(fionaScene->pOptProx != 0 && fionaScene->clipProxPlane != -1)
	{
		if(fionaConf.currentJoystick.x != 0.f || fionaConf.currentJoystick.z != 0.f)
		{
			static const char *sClipProps[6] = {"clipRight", "clipLeft", "clipFront", "clipBack", "clipTop", "clipBottom"};
			voreen::Property *p = fionaScene->pOptProx->getProperty(sClipProps[fionaScene->clipProxPlane]);
			if(p != 0)
			{
				voreen::FloatProperty *pF = dynamic_cast<voreen::FloatProperty*>(p);
				if(pF)
				{
					float fVal = pF->get();
					if(fionaScene->clipProxPlane == 0 || fionaScene->clipProxPlane == 1)
					{
						fVal -= fionaConf.currentJoystick.x;
					}
					else if(fionaScene->clipProxPlane == 2 || fionaScene->clipProxPlane == 3)
					{
						fVal += fionaConf.currentJoystick.z;
					}
					else
					{
						fVal -= fionaConf.currentJoystick.z;
					}

					pF->set(fVal);
				}
			}
		}
	}*/
}

void VRClip::ButtonUp(void)
{
	FionaVoreen *fionaScene = static_cast<FionaVoreen*>(m_scene);
	if(fionaScene->hasClipPlanes())
	{
		static const char* names[12] = {"enableLeftX", "", "enableRightX", "", "enableBottomY", "", "enableTopY", "", "enableBackZ", "", "enableFrontZ", ""};

		//switch amongst which plane is the current clip plane
		//render the outline as we change amongst the planes..
		int before = fionaScene->getClipPlane();

		if(before != -1)
		{
			fionaScene->setClipProperty(names[before], true);
		}
		else
		{
			fionaScene->setClipProperty("showInnerBB", true);
			fionaScene->turnOnJoystickAction(true);
		}

		if(before == -1)
		{
			fionaScene->setClipPlane(0);
		}
		else
		{
			before += 2;
			fionaScene->setClipPlane(before);
		}

		if(before > 10)
		{
			fionaScene->setClipPlane(-1);
		}
		
		int after = fionaScene->getClipPlane();

		if(after != -1)
		{
			fionaScene->setClipProperty(names[after], true);
		}
		else
		{
			fionaScene->setClipProperty("showInnerBB", false);
			fionaScene->turnOnJoystickAction(false);
		}
	}
	/*else if(fionaScene->pOptProx != 0)
	{
		fionaScene->clipProxPlane = fionaScene->clipProxPlane + 1;
		if(fionaScene->clipProxPlane == 6)
		{
			fionaScene->clipProxPlane = -1;
		}
	}*/
}

void VRResetClip::ButtonUp(void)
{
	FionaVoreen *fionaScene = static_cast<FionaVoreen*>(m_scene);
	//get which plane is currently active (if any)
	if(fionaScene->getClipPlane() != -1)
	{
		voreen::FloatProperty *pClip = 0;
		int grabbed = fionaScene->getClipPlane();
		//left
		pClip = fionaScene->getClipProperty("leftClipPlane");
		pClip->set(pClip->getMaxValue());
		//right
		pClip = fionaScene->getClipProperty("rightClipPlane");
		pClip->set(pClip->getMinValue());
		//front
		pClip = fionaScene->getClipProperty("bottomClipPlane");
		pClip->set(pClip->getMinValue());
		//back
		pClip = fionaScene->getClipProperty("topClipPlane");
		pClip->set(pClip->getMaxValue());
		//bottom
		pClip = fionaScene->getClipProperty("frontClipPlane");
		pClip->set(pClip->getMinValue());
		//top
		pClip = fionaScene->getClipProperty("backClipPlane");
		pClip->set(pClip->getMaxValue());
	}
}

void VRToggleInnerBox::ButtonUp(void)
{
	FionaVoreen *fionaScene = static_cast<FionaVoreen*>(m_scene);
	static bool bOn = true;
	fionaScene->setClipProperty("showInnerBB", bOn);
	bOn = !bOn;
}