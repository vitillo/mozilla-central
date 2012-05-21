/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * a list of the recomputation that needs to be done in response to a
 * style change
 */

#include "nsStyleChangeList.h"
#include "nsStyleConsts.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsCRT.h"

static const PRUint32 kGrowArrayBy = 10;

nsStyleChangeList::nsStyleChangeList()
  : mArray(mBuffer),
    mArraySize(kStyleChangeBufferSize),
    mCount(0)
{
  MOZ_COUNT_CTOR(nsStyleChangeList);
}

nsStyleChangeList::~nsStyleChangeList()
{
  MOZ_COUNT_DTOR(nsStyleChangeList);
  Clear();
}

nsresult 
nsStyleChangeList::ChangeAt(PRInt32 aIndex, nsIFrame*& aFrame, nsIContent*& aContent, 
                            nsChangeHint& aHint) const
{
  if ((0 <= aIndex) && (aIndex < mCount)) {
    aFrame = mArray[aIndex].mFrame;
    aContent = mArray[aIndex].mContent;
    aHint = mArray[aIndex].mHint;
    return NS_OK;
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

nsresult 
nsStyleChangeList::ChangeAt(PRInt32 aIndex, const nsStyleChangeData** aChangeData) const
{
  if ((0 <= aIndex) && (aIndex < mCount)) {
    *aChangeData = &mArray[aIndex];
    return NS_OK;
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

nsresult 
nsStyleChangeList::AppendChange(nsIFrame* aFrame, nsIContent* aContent, nsChangeHint aHint)
{
  NS_ASSERTION(aFrame || (aHint & nsChangeHint_ReconstructFrame),
               "must have frame");
  NS_ASSERTION(aContent || !(aHint & nsChangeHint_ReconstructFrame),
               "must have content");
  // XXXbz we should make this take Element instead of nsIContent
  NS_ASSERTION(!aContent || aContent->IsElement(),
               "Shouldn't be trying to restyle non-elements directly");
  NS_ASSERTION(!(aHint & nsChangeHint_ReflowFrame) ||
               (aHint & nsChangeHint_NeedReflow),
               "Reflow hint bits set without actually asking for a reflow");

  if ((0 < mCount) && (aHint & nsChangeHint_ReconstructFrame)) { // filter out all other changes for same content
    if (aContent) {
      for (PRInt32 index = mCount - 1; index >= 0; --index) {
        if (aContent == mArray[index].mContent) { // remove this change
          aContent->Release();
          mCount--;
          if (index < mCount) { // move later changes down
            ::memmove(&mArray[index], &mArray[index + 1], 
                      (mCount - index) * sizeof(nsStyleChangeData));
          }
        }
      }
    }
  }

  PRInt32 last = mCount - 1;
  if ((0 < mCount) && aFrame && (aFrame == mArray[last].mFrame)) { // same as last frame
    NS_UpdateHint(mArray[last].mHint, aHint);
  }
  else {
    if (mCount == mArraySize) {
      PRInt32 newSize = mArraySize + kGrowArrayBy;
      nsStyleChangeData* newArray = new nsStyleChangeData[newSize];
      if (newArray) {
        memcpy(newArray, mArray, mCount * sizeof(nsStyleChangeData));
        if (mArray != mBuffer) {
          delete [] mArray;
        }
        mArray = newArray;
        mArraySize = newSize;
      }
      else {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    mArray[mCount].mFrame = aFrame;
    mArray[mCount].mContent = aContent;
    if (aContent) {
      aContent->AddRef();
    }
    mArray[mCount].mHint = aHint;
    mCount++;
  }
  return NS_OK;
}

void
nsStyleChangeList::Clear()
{
  for (PRInt32 index = mCount - 1; index >= 0; --index) {
    nsIContent* content = mArray[index].mContent;
    if (content) {
      content->Release();
    }
  }
  if (mArray != mBuffer) {
    delete [] mArray;
    mArray = mBuffer;
    mArraySize = kStyleChangeBufferSize;
  }
  mCount = 0;
}

