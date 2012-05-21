/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PCOMContentPermissionRequestChild_h
#define PCOMContentPermissionRequestChild_h

#include "mozilla/dom/PContentPermissionRequestChild.h"
// Microsoft's API Name hackery sucks
#undef CreateEvent

/*
  PContentPermissionRequestChild implementations also are
  XPCOM objects.  Addref() is called on their implementation
  before SendPCOntentPermissionRequestConstructor is called.
  When Dealloc is called, IPDLRelease() is called.
  Implementations of this method are expected to call
  Release() on themselves.  See Bug 594261 for more
  information.
 */
class PCOMContentPermissionRequestChild : public mozilla::dom::PContentPermissionRequestChild {
public:
  virtual void IPDLRelease() = 0;
};

#endif
