
/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2015 - 2019 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
 *
 * This class implements a set of consecutive steps within either a row
 * or column of a callout or multi-step.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "range.h"
#include "ranges_item.h"
#include "pointer.h"
#include "step.h"
#include "reserve.h"
#include "meta.h"
#include <QMenu>

Range::Range(
  Steps        *_parent,
  AllocEnc      _allocType,
  FreeFormMeta  _freeform)
{
  parent       = _parent;
  allocType    = _allocType;
  freeform     = _freeform;
  relativeType = RangeType;
  stepSpacing  = 0;
}

Range::~Range()
{
  for (int i = 0; i < list.size(); i++) {
    AbstractRangeElement *re = list[i];
    delete re;
  }
  list.clear();
  for (int i = 0; i < rangeDividerPointerList.size(); i++) {
    Pointer *p = rangeDividerPointerList[i];
    delete p;
  }
  rangeDividerPointerList.clear();

  for (int i = 0; i < stepDividerPointerList.size(); i++) {
    Pointer *p = stepDividerPointerList[i];
    delete p;
  }
  stepDividerPointerList.clear();
}

void Range::append(AbstractRangeElement *gi)
{
  list.append(gi);
}

/*********************************************
 *
 * Range functions
 *
 * Range
 *
 * range contains a list of steps (possibly reserve too)
 *
 ********************************************/

void Range::sizeMargins(
  int cols[NumPlaces],
  int colsMargin[][2],
  int margins[NumPlaces])
{
  int active = 0;

  for (int i = 0; i < NumPlaces; i++) {
    margins[i] = 0;
    if (cols[i]) {
      ++active;
    }
  }

  for (int i = TblSn0; active && i < TblCo5 - 1; i++) {
    if (cols[i] && --active) {
      margins[i] = qMax(colsMargin[i][1],colsMargin[i+1][0]);
      for (int j = i + 1; j < NumPlaces; j++) {
        if (cols[j]) {
          margins[i] = qMax(colsMargin[i][1],colsMargin[j][0]);
          break;
        }
      }
    }
  }
}

int Range::sizeRangeDividers(int axis) {
    SepData divider = sepMeta.valuePixels();
    int dividerSizeAdjust = 0;
    for (int i = 0; i < list.size()-1; i++) {
      if (list[i]->relativeType == StepType) {
        dividerSizeAdjust += qRound(2*divider.margin[axis] + divider.thickness);
      }
    }
    return dividerSizeAdjust;
}

/*
 * to size a range Vertically
 *   1. determine the height of each step
 *   2. place the step Vertically
 *   3. determine the width of each column
 *   4. place each step Horizontally
 *      determine width of range
 *   5. place each step Vertically
 *      determine height of range
 *
 * to size a range Horizontally
 *   1. determine the width of each step
 *   2. place the step Horizontally
 *   3. determine the height of each row
 *   4. place each step Vertically
 *      determine height of each range
 *   5. place each step Horizontally
 *      determine width of range
 */

void Range::sizeitVert()
{
  int     rows[NumPlaces];
  int     rowsMargin[NumPlaces][2];
  int     cols[NumPlaces];
  int     colsMargin[NumPlaces][2];
  int     margins[NumPlaces];

  /* we accumulate the widest of the columns within the steps */

  for (int i = 0; i < NumPlaces; i++) {
    cols[i] = 0;
    colsMargin[i][0] = 0;
    colsMargin[i][1] = 0;
  }

  /* size each step, and place its components Vertically */

  int lastMargin = 0;

  size[YY] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      for (int i = 0; i < NumPlaces; i++) {
        rows[i] = 0;
        rowsMargin[i][0] = 0;
        rowsMargin[i][1] = 0;
      }

      /* size the step both Vertically and Horizontally */

      step->sizeit(rows,cols,rowsMargin,colsMargin,XX,YY);
      
      sizeMargins(rows,rowsMargin,margins);

      /* place the step components Vertically */

      step->placeit(rows,margins,YY);
      
      /* accumulate minimum height of column */

      int topMargin = 0, botMargin = 0;

      step->maxMargin(topMargin,botMargin,YY);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      /*  adjust size height */

      size[YY] += step->size[YY] + lastMargin;

#ifdef QT_DEBUG_MODE
      logTrace() << "\nRange.cpp Range::sizeVert()"
                 << "\nRangeDivider Vertical Step [" << step->stepNumber.number << "] Height:"
                 << "\nStep::size YY     [" << step->size[YY] << "]"
                    ;
#endif
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      size[YY] += reserve->size[YY];
    }
  }

  size[YY] -= lastMargin;

  /* place all step's components into columns */

  /*********************************/
  /* determine the margins needed  */
  /*********************************/
  
  sizeMargins(cols,colsMargin,margins);

  size[XX] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeit(cols,margins,XX, true);

      if (step->size[XX] > size[XX]) {
        size[XX] = step->size[XX];
      }
    }
  }

  /* Now, for step components that are "inside" */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeInside();
    }
  }
  
  setBoundingSize();

#ifdef QT_DEBUG_MODE
  logTrace() << "\nRange.cpp Range::sizeVert()"
             << "\nRangeDivider Vertical Range Size:"
             << "\nRange::size XX     [" << size[XX] << "]"
             << "\nRange::size YY     [" << size[YY] << "]"
                ;
#endif
}

/*
 * we know the tallest of all the columns, so evenly
 * space the steps within the column
 */
void Range::placeit(int max, int x, int y)
{
  /* determine the spacing needed */

  int top = 0;
  stepSpacing = 0;

  if (list.size() < 3) {
    stepSpacing = (max - size[y])/(list.size() + 1);
    top = stepSpacing;
  } else {
    stepSpacing = (max - size[y])/(list.size() - 1);
  }

#ifdef QT_DEBUG_MODE
//  logNotice() << "\nRange.cpp Range::placeit()"
//              << "\nRangeDivider Range Spacing and Size Height (Start):"
//              << "\nRange::stepSpacing [" << stepSpacing << "]"
//              << "\nRange::size YY     [" << size[y] << "]"
//                 ;
#endif
  /* evenly space the steps Vertically */

  int lastMargin = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->loc[x] = 0;
      step->loc[y] = top;

//      logNotice() << "\nRangeDivider Range Top for Step [" << step->stepNumber.number << "]:"
//                  << "\nStep::top         [" << top << "]"
//                     ;

      top += step->size[y] + stepSpacing;

      /* accumulate minimum height of column */

      int topMargin, botMargin;

      step->maxMargin(topMargin,botMargin,YY);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      top += lastMargin;

//      logNotice() << "\nRangeDivider adjusted Top and Size (top - stepSpacing) Height for Step [" << step->stepNumber.number << "]:"
//                  << "\nRange::adj top     [" << top << "] top += step->size[y] + stepSpacing + lastMargin"
//                  << "\nRange::adj size    [" << top - stepSpacing << "] top - stepSpacing"
//                     ;

    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      if (reserve) {
        top += reserve->size[y];
      }
    }
    size[y] = top - stepSpacing;
  }
  size[y] = max;
  setBoundingSize();
}

void Range::sizeitHoriz()
{
  int     rows[NumPlaces];
  int     rowsMargin[NumPlaces][2];
  int     cols[NumPlaces];
  int     colsMargin[NumPlaces][2];
  int     margins[NumPlaces];

  /* we accumulate the tallest of the rows within the steps */

  for (int i = 0; i < NumPlaces; i++) {
    rows[i] = 0;
    rowsMargin[i][0] = 0;
    rowsMargin[i][1] = 0;
  }

  /* size each step, and place its components Horizontally */

  int lastMargin = 0;

  size[XX] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      for (int i = 0; i < NumPlaces; i++) {
        cols[i] = 0;
        colsMargin[i][0] = 0;
        colsMargin[i][1] = 0;
      }

      /* size the step both Vertically and Horizontally */

      step->sizeit(rows,cols,rowsMargin,colsMargin,XX,YY);

      sizeMargins(cols,colsMargin,margins);

      /* place the step components Horizontally */

      step->placeit(cols,margins,XX);

      /* accumulate minimum width of row */

      int topMargin, botMargin;

      step->maxMargin(topMargin,botMargin,XX);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      /*  adjust size height */

      size[XX] += step->size[XX] + lastMargin;

//      logTrace() << "\nRangeDivider Horizontal Step [" << step->stepNumber.number << "] Width:"
//                 << "\nStep::size XX     [" << step->size[XX] << "]"
//                    ;

    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      size[XX] += reserve->size[XX];
    }
  }

  size[XX] -= lastMargin;

  /* place all step's components into rows */

  sizeMargins(rows,rowsMargin,margins);

  size[YY] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeit(rows,margins,YY, true);

      if (step->size[YY] > size[YY]) {
        size[YY] = step->size[YY];
      }
    }
  }

  /* Now, for step components that are "inside" */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeInside();
    }
  }

  setBoundingSize();

  logTrace() << "\nRangeDivider Horizontal Range Size:"
             << "\nRange::size XX     [" << size[XX] << "]"
             << "\nRange::size YY     [" << size[YY] << "]"
                ;
}

/*****************************************************************************
 *
 * freeform
 *
 ****************************************************************************/

void Range::sizeitFreeform(
  int xx,
  int yy,
  int base,
  int justification)
{
  int maxLeft = 50000, maxRight = - 50000;
  int left, right;

  size[xx] = 0;
  size[yy] = margin.valuePixels(yy);

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->sizeitFreeform(xx,yy,base, justification, left, right);
      if (left < maxLeft) {
        maxLeft = left;
      }
      if (right > maxRight) {
        maxRight = right;
      }
      size[yy] += step->size[yy] + step->margin.valuePixels(yy);
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);

      size[yy] += reserve->size[yy];
    }
  }

  leftAdjust = -maxLeft;

  size[xx] += maxRight - maxLeft + 2*margin.valuePixels(xx);
  setBoundingSize();
}

/*
 * we know the tallest of all the columns, so evenly
 * space the steps within the column
 */
void Range::placeitFreeform(
  int xx,
  int yy,
  int max,
  int justification)
{
  /* determine the spacing needed */

  int spacing = (max - size[yy])/(list.size() + 1);

  int y = margin.valuePixels(yy) + spacing;

  /* evenly space the steps Vertically */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->loc[yy] = y;

      /* Horizontal placement */

      switch (justification) {
        case Left:
        case Top:
          step->loc[xx] = margin.valuePixels(xx);
        break;
        case Center:
          step->loc[xx] = size[xx]/2 + margin.valuePixels(xx);
        break;
        default:
          step->loc[xx] = size[xx] + margin.valuePixels(xx);
        break;
      }

      y += step->size[yy] + spacing + step->margin.valuePixels(yy);
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      y += reserve->size[yy];
    }
    size[yy] = y - spacing;
  }
  size[yy] = max;
  setBoundingSize();
}

/******************************************************************************
 * Divider pointer item routines
 *****************************************************************************/

void Range::appendDividerPointer(
  const Where       &here,
  PointerMeta       &pointerMeta,
  PointerAttribMeta &pointerAttrib,
  QGraphicsView    *_view,
  int                stepNum,
  bool               rd)
{
  int pid = rd ? rangeDividerPointerList.size()  + 1 :
                 stepDividerPointerList.size() + 1;
  PointerAttribMeta pam = pointerAttrib;
  Pointer *pointer = new Pointer(pid,here,stepNum,pointerMeta);
  pam.setDefaultColor(sepMeta.value().color);
  pointer->setPointerAttribInches(pam);
  if (rd)
      rangeDividerPointerList.append(pointer);
  else
      stepDividerPointerList.append(pointer);
  view    = _view;
}

/******************************************************************************
 * Qt Graphics Scene routines
 *****************************************************************************/

void Range::addGraphicsItems(
  int xx,
  int yy,
  Meta *meta,
  PlacementType  parentRelativeType,
  QGraphicsItem *parent)
{
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);
      step->addGraphicsItems(xx+loc[XX],yy+loc[YY],meta,parentRelativeType,parent,false);
    }
  }
}
