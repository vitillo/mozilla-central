/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsICSSFrameConstructor_h___
#define nsICSSFrameConstructor_h___

#include "nsISupports.h"

class nsIDocument;

// {1691E1F6-EE41-11d4-9885-00C04FA0CF4B}
#define NS_ICSSFRAMECONSTRUCTOR_IID \
{ 0x1691e1f6, 0xee41, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

class nsICSSFrameConstructor : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICSSFRAMECONSTRUCTOR_IID)

  NS_IMETHOD Init(nsIDocument* aDocument) = 0;
};

#endif /* nsICSSFrameConstructor_h___ */
