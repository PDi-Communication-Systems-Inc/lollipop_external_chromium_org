// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#ifndef V8TestCallbackInterface_h
#define V8TestCallbackInterface_h

#include "bindings/tests/idls/TestCallbackInterface.h"
#include "bindings/v8/ActiveDOMCallback.h"
#include "bindings/v8/DOMWrapperWorld.h"
#include "bindings/v8/ScopedPersistent.h"

namespace WebCore {

class V8TestCallbackInterface FINAL : public TestCallbackInterface, public ActiveDOMCallback {
public:
    static PassOwnPtr<V8TestCallbackInterface> create(v8::Handle<v8::Function> callback, ScriptState* scriptState)
    {
        return adoptPtr(new V8TestCallbackInterface(callback, scriptState));
    }

    virtual ~V8TestCallbackInterface();

    virtual void voidMethod() OVERRIDE;
    virtual bool booleanMethod() OVERRIDE;
    virtual void voidMethodBooleanArg(bool boolArg) OVERRIDE;
    virtual void voidMethodSequenceArg(const Vector<RefPtr<TestInterfaceEmpty> >& sequenceArg) OVERRIDE;
    virtual void voidMethodFloatArg(float floatArg) OVERRIDE;
    virtual void voidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) OVERRIDE;
    virtual void voidMethodTestInterfaceEmptyStringArg(TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) OVERRIDE;
    virtual void callbackWithThisValueVoidMethodStringArg(ScriptValue thisValue, const String& stringArg) OVERRIDE;
    virtual void customVoidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) OVERRIDE;
    virtual void voidMethodWillBeGarbageCollectedSequenceArg(const WillBeHeapVector<RefPtrWillBeMember<TestInterfaceWillBeGarbageCollected> >& sequenceArg) OVERRIDE;
    virtual void voidMethodWillBeGarbageCollectedArrayArg(const WillBeHeapVector<RefPtrWillBeMember<TestInterfaceWillBeGarbageCollected> >& arrayArg) OVERRIDE;
private:
    V8TestCallbackInterface(v8::Handle<v8::Function>, ScriptState*);

    ScopedPersistent<v8::Function> m_callback;
    RefPtr<ScriptState> m_scriptState;
};

}
#endif // V8TestCallbackInterface_h
