/*	see copyright notice in squirrel.h */
#ifndef _SQOBJECT_H_
#define _SQOBJECT_H_

#include "squirrel.h"
#include "squtils.h"

struct SQSharedState;

struct SQRefCounted
{
	SQRefCounted() { _uiRef = 0; _weakref = NULL; _context = NULL; _extraData = NULL; }

	virtual ~SQRefCounted();
	virtual void Release() = 0;

	//SQWeakRef* GetWeakRef(SQObjectType type);

	SQUnsignedInteger _uiRef;
	SQObject* _weakref; // this is not an sqobject!

	// vm context used to track total memory usage of the vm, indexes
	// into  s_perVmScriptMemUsage in SQWeakRef::Release(), see:
	// - r5apex + B1C7DE] for assignment.
	// - [r5apex + B2CAF8] for usage.
	SQInteger _context;
	void* _extraData; // unknown
};

struct SQWeakRef : SQRefCounted
{
	void Release();
	SQObject _obj;
};

#define _realval(o) (sq_type((o)) != OT_WEAKREF?(SQObject)o:_weakref(o)->_obj)

#define __AddRef(type, unval) \
	if(ISREFCOUNTED(type)) \
	{ \
		unval.pRefCounted->_uiRef++; \
	}

#define __Release(type, unval) \
	if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)<=0)) \
	{ \
		unval.pRefCounted->Release(); \
	}

#define __ObjAddRef(obj) { \
	SQ_VALIDATE_REF_COUNT( obj ); \
	(obj)->_uiRef++; \
}

#define __ObjRelease(obj) { \
	if((obj)) {	\
		if( ( -- ((obj)->_uiRef) ) <= 0 ) \
		{ \
			SQ_VALIDATE_REF_COUNT( obj ); \
			(obj)->Release(); \
		} \
		(obj) = NULL;	\
	} \
}

//#define type(obj) ((obj)._type)
// VALVE_BUILD -- define a version of 'type' that is less name-space polluting so that we can #undef
// and redef type around VS 2013 STL header files.
#define sq_type(obj) ((obj)._type)
#define is_delegable(t) (type(t)&SQOBJECT_DELEGABLE)
#define raw_type(obj) _RAW_TYPE((obj)._type)

#define _bool(obj) (!!(obj)._unVal.nInteger)
#define _integer(obj) ((obj)._unVal.nInteger)
#define _float(obj) ((obj)._unVal.fFloat)
#define _string(obj) ((obj)._unVal.pString)
#define _table(obj) ((obj)._unVal.pTable)
#define _array(obj) ((obj)._unVal.pArray)
#define _closure(obj) ((obj)._unVal.pClosure)
#define _generator(obj) ((obj)._unVal.pGenerator)
#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
#define _userdata(obj) ((obj)._unVal.pUserData)
#define _userpointer(obj) ((obj)._unVal.pUserPointer)
#define _thread(obj) ((obj)._unVal.pThread)
#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
#define _class(obj) ((obj)._unVal.pClass)
#define _instance(obj) ((obj)._unVal.pInstance)
#define _delegable(obj) ((SQDelegable *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _vector3d(obj) ((Vector3D*)&(obj)._pad)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) (obj)._unVal.pUserData->_val

#define tofloat(num) ((sq_type(num)==OT_INTEGER)?(SQFloat)_integer(num):_float(num))
#define tointeger(num) ((sq_type(num)==OT_FLOAT)?(SQInteger)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
struct SQObjectPtr : public SQObject
{
	SQObjectPtr()
	{
		_type = OT_NULL;
		_pad = NULL;
		_unVal.pUserPointer = nullptr;
	}

	SQObjectPtr(const SQObjectPtr& o)
	{
		_type = o._type;
		_pad = o._pad;
		_unVal = o._unVal;
		__AddRef(_type, _unVal);
	}

	SQObjectPtr(const SQObject& o)
	{
		_type = o._type;
		_pad = NULL;
		_unVal = o._unVal;
		__AddRef(_type, _unVal);
	}

	SQObjectPtr(SQString* pString)
	{
		assert(pString);

		_type = OT_STRING;
		_pad = NULL;
		_unVal.pString = pString;
		__AddRef(_type, _unVal);
	}

	SQObjectPtr(bool bBool)
	{
		_type = OT_BOOL;
		_pad = NULL;
		_unVal.nInteger = bBool ? 1 : 0;
	}

	SQObjectPtr(SQInteger nInteger)
	{
		_type = OT_INTEGER;
		_pad = NULL;
		_unVal.nInteger = nInteger;
	}

	SQObjectPtr(SQFloat fFloat)
	{
		_type = OT_FLOAT;
		_pad = NULL;
		_unVal.fFloat = fFloat;
	}

	inline ~SQObjectPtr()
	{
		__Release(_type, _unVal);
	}

	inline SQObjectPtr& operator=(const SQObjectPtr& obj)
	{
		SQObjectType oldType = _type;
		SQObjectValue oldVal = _unVal;

		_type = obj._type;
		_pad = obj._pad;
		_unVal = obj._unVal;

		__AddRef(_type, _unVal);
		__Release(oldType, oldVal);

		return *this;
	}
};

static SQObjectPtr _null_;

struct SQCollectable : public SQRefCounted {
	SQCollectable* _next;
	SQCollectable* _prev;
	SQSharedState* _sharedstate;
	virtual void Release() = 0;
	virtual void Mark(SQCollectable** chain) = 0;
	//void UnMark();
	virtual void Finalize() = 0;
	//static void AddToChain(SQCollectable** chain, SQCollectable* c);
	//static void RemoveFromChain(SQCollectable** chain, SQCollectable* c);
};

#define CHAINABLE_OBJ SQCollectable

struct SQDelegable : public CHAINABLE_OBJ {
	//bool SetDelegate(SQTable* m);

	virtual void DumpToString(SQChar* buf, SQUnsignedInteger len, SQInteger start, SQInteger end) = 0;

	//enum SQMetaMethod; // TODO
	virtual bool GetMetaMethod(SQVM* v, /*SQMetaMethod*/ int mm, SQObjectPtr& res) = 0;
	SQTable* _delegate;
};

typedef sqvector<SQObjectPtr> SQObjectPtrVec;
typedef sqvector<SQInteger> SQIntVec;

const SQChar* IdType2Name(const SQObjectType type);

#endif //_SQOBJECT_H_
