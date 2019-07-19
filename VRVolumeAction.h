#ifndef _VRVOLUMEACTION_H_
#define _VRVOLUMEACTION_H_

//Ross Tredinnick - UWLEL 2013

#include "VRAction.h"


class VRClip : public VRWandAction
{
public:
	VRClip() : VRWandAction("volume_clip_enable") {}
	virtual ~VRClip() {}

	virtual void ButtonUp(void);

	/*virtual	void	ButtonDown(void) {}
	virtual void	ButtonUp(void) {}
	virtual void	WandMove(void) {}
	virtual void	JoystickStart(void) {}
	virtual void	JoystickMove(void) {}
	virtual void	JoystickStop(void) {}
	virtual void	DrawCallback(void) {}*/

protected:

private:


};

class VRToggleInnerBox : public VRWandAction
{
public:
	VRToggleInnerBox() : VRWandAction("volume_toggle_inner_box") {}
	~VRToggleInnerBox() {}

	virtual void ButtonUp(void);

protected:

private:

};

class VRAdjustClip : public VRWandAction
{
public:
	VRAdjustClip() : VRWandAction("volume_clip_adjust") {}
	~VRAdjustClip() {}

	virtual void	JoystickMove(void);

protected:

private:

};


class VRResetClip : public VRWandAction
{
public:
	VRResetClip() : VRWandAction("volume_clip_reset") {}
	~VRResetClip() {}

	virtual void ButtonUp(void);

protected:

private:

};
#endif