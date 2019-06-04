// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFLINE_H_
#define XFA_FXFA_CXFA_FFLINE_H_

#include "xfa/fxfa/cxfa_ffwidget.h"

class CXFA_FFLine final : public CXFA_FFWidget {
 public:
  explicit CXFA_FFLine(CXFA_Node* pNode);
  ~CXFA_FFLine() override;

  // CXFA_FFWidget
  void RenderWidget(CXFA_Graphics* pGS,
                    const CFX_Matrix& matrix,
                    uint32_t dwStatus) override;

 private:
  void GetRectFromHand(CFX_RectF& rect,
                       XFA_AttributeEnum iHand,
                       float fLineWidth);
};

#endif  // XFA_FXFA_CXFA_FFLINE_H_
