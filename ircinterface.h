/**********************************************************************
FILE: rcinterface.h
DESCRIPTION: RailClone interface for external renderers
CREATED BY: CQG
HISTORY:	03/09/2013 - First Version
					03/01/2017 - added IRCSetEngineFeatures, TRCInstance improved to V300

	Copyright (c) iToo Software. All Rights Reserved.

************************************************************************

The following interface can be used by third party render engines to instantiate the RailClone items, in the following way:

1) Using the static RailClone interface, Register the current render engine as supported for instancing.
   Strictly, this funcion only need to be inkoked one time by Max session, but it's ok if you call it more times:
	
		IRCStaticInterface *isrc = GetRCStaticInterface();
		isrc->IRCRegisterEngine();
		TRCEngineFeatures features;
		if(isrc->functions.Count() > 2)		// safety check to avoid crash if RailClone plugin is not V3
			isrc->IRCSetEngineFeatures(&features);

		After calling this, "features.rcAPIversion" returns the RailClone API version, as defined in TRAILCLONE_API_VERSION (see definition in code below).
		You can use it to know what RailClone version is installed, and what features are supported. 
		In RailClone 2.x and before, rcAPIversion will be initialized to zero (since IRCRegisterEngine was not defined).

At the rendering loop, repeat for each RailClone object:

2) Get a pointer to the IRCInterface interface:

		IRCInterface *irc = GetRCInterface(node->GetObjectRef());
		if (irc)
			{
	    ...
			}

3) Call to irc->RenderBegin(t). It prepares the object for rendering. If you have some pre-rendering phase, make the call from there.

4) For each segment that can be instanced, RailClone keeps internally a copy of its mesh. Use the following function to get an array of pointers to these meshes:

	int numMeshes;
	Mesh **pmesh = (Mesh **) irc->IRCGetMeshes(numMeshes);
	if (pmesh && numMeshes)
		{
		for (int i = 0; i < numMeshes; i++, pmesh++)
			{
			if (*pmesh)
				{
				Mesh &mesh = *pmesh;
				...
				}
			}
		}

	In the above loop, you must prepare each of these meshes for rendering. Usually converting them to the native geometry format of your engine.
	The number of meshes will be stored in "numMeshes". If the function fails for some reason, or there is not renderable geometry, it will return NULL. 
	The meshes are zero aligned and not transformed, since the transformation matrix depends of each instance (see next step).

5) Generate the array of instances (instances of the meshes generated in step 5), and get a pointer to it:

	int numInstances;
	TRCInstance *inst = (TRCInstance *) itrees->IRCGetInstances(numInstances);
	if (inst && numInstances)
		{
		for (int i = 0; i < numInstances; i++, inst++)
			{
			if(inst->mesh)
				{
				...
				}
			}
		}

	The number of instances will be stored in "numInstances". If the function fails for some reason, or there is not renderable geometry, it will return NULL. 
	
	Each "TRCInstance" stores full information about the instance, incluing the source mesh, transformation matrix and more. See the 
	class definition below in this header file. Note: In some cases TRCInstance->mesh would be NULL, you must handle this case and skip it.

	- The transformation matrix is on local coordinates of the RailClone object. Just multiply it by the INode TM to get the world coordinates of the instance.
	- RailClone doesn't apply separated materials to the instances, use the same material assigned to the RailClone object.
	- The first item stores the geometry of the RailClone object that is not instantiable. This item is unique, and uses the first mesh returned by irc->IRCGetMeshes.
	- In case that Display->Render->Use Geometry Shader is off, there will be an unique item holding the geometry of the full RC object.

6) Clear the arrays:

	irc->IRCClearInstances();
	irc->IRCClearMeshes();

7) At the render's end, call to irc->RenderEnd(t). This function builds the object for viewport, clearing the rendering data.


**********************************************************************/

#ifndef __IRCINTERFACE__H
#define __IRCINTERFACE__H

#include "ifnpub.h"

// Forest Class_ID
#define TRAIL_CLASS_ID	Class_ID(0x39712def, 0x10a72959)
// API Version
#define TRAILCLONE_API_VERSION			300

///////////////////////////////////////////////////////////////////////////////////////////
// RailClone Interface

#define RC_MIX_INTERFACE Interface_ID(0x54617e51, 0x67454c0c)
#define GetRCInterface(obj) ((IRCInterface*) obj->GetInterface(RC_MIX_INTERFACE))

// function IDs
enum { rc_segments_updateall, rc_getmeshes, rc_clearmeshes, rc_getinstances, rc_clearinstances, rc_renderbegin, rc_renderend, rc_getstyledesc, rc_setstyledesc, rc_resetcreatedversion, rc_setcreatedversion };


class TRCEngineFeatures
	{
	public:
	int renderAPIversion;			// this value is initialized in constructor with TRAILCLONE_API_VERSION. Used by RC to identify API version used by renderer
	int rcAPIversion;					// after calling to IRCRegisterEngine, this value returns API version from the RailClone side. Use it from renderer to check API compatibility.
	int reserved[4];				

	void init() { renderAPIversion = TRAILCLONE_API_VERSION; rcAPIversion = reserved[3] = reserved[2] = reserved[1] = reserved[0] = 0; }
	TRCEngineFeatures() { init(); }
	};


class TRCInstanceV2
	{
	public:
	Matrix3 tm;						// full transformation for the instance
	Mesh *mesh;						// source mesh
	};


class TRCInstanceV300: public TRCInstanceV2
	{
	public:
	MaxSDK::Array<int> const *matid;	// pointer to an array with subtitutions of Material ID for this instance
																		// it includes a pair number of integers: first value is the original ID, and next one is new material ID
																		// Note: pointer may be NULL if there are not substitutions
	int reserved[5];
	};


typedef TRCInstanceV300 TRCInstance;


class IRCInterface : public FPMixinInterface 
	{
	BEGIN_FUNCTION_MAP
		VFN_2(rc_segments_updateall, IRCSegmentsUpdateAll, TYPE_INT, TYPE_INT);
		FN_1(rc_getmeshes, TYPE_INTPTR, IRCGetMeshes, TYPE_INT);
		VFN_0(rc_clearmeshes, IRCClearMeshes);
		FN_1(rc_getinstances, TYPE_INTPTR, IRCGetInstances, TYPE_INT);
		VFN_0(rc_clearinstances, IRCClearInstances);
		VFN_1(rc_renderbegin, IRCRenderBegin, TYPE_TIMEVALUE);
		VFN_1(rc_renderend, IRCRenderEnd, TYPE_TIMEVALUE);
		FN_0(rc_getstyledesc, TYPE_TSTR_BR, IRCGetStyleDesc);
		VFN_1(rc_setstyledesc, IRCSetStyleDesc, TYPE_TSTR_BR);
		VFN_0(rc_resetcreatedversion, IRCResetCreatedVersion);
		VFN_1(rc_setcreatedversion, IRCSetCreatedVersion, TYPE_INT);
	END_FUNCTION_MAP

	virtual void IRCSegmentsUpdateAll(int n1, int n2) = 0;
	virtual INT_PTR IRCGetMeshes(int &numNodes) = 0;
	virtual void IRCClearMeshes() = 0;
	virtual INT_PTR IRCGetInstances(int &numNodes) = 0;
	virtual void IRCClearInstances() = 0;
	virtual void IRCRenderBegin(TimeValue t) = 0;
	virtual void IRCRenderEnd(TimeValue t) = 0;
	virtual TSTR &IRCGetStyleDesc() = 0;
	virtual void IRCSetStyleDesc(TSTR &desc) = 0;
	virtual void IRCResetCreatedVersion() = 0;
	virtual void IRCSetCreatedVersion(int ver) = 0;

	FPInterfaceDesc* GetDesc();	
	};


///////////////////////////////////////////////////////////////////////////////////////////
// RailClone Static Interface

#define RC_STATIC_INTERFACE Interface_ID(0x2bd6594f, 0x5e6509d6)

#define GetRCStaticInterface()	(IRCStaticInterface *) ::GetInterface(GEOMOBJECT_CLASS_ID, TRAIL_CLASS_ID, RC_STATIC_INTERFACE)

enum { rc_registerengine, rc_version, rc_setenginefeatures };

class IRCStaticInterface : public FPStaticInterface 
	{
	public:
	virtual void IRCRegisterEngine() = 0;
	virtual int IRCVersion() = 0;
	virtual void IRCSetEngineFeatures(INT_PTR pdata) = 0;

	FPInterfaceDesc* GetDesc();	
	};



#endif
