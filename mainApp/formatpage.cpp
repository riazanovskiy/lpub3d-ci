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

/***************************************************************************
 *
 * This file invokes the traverse function to count pages, and to gather
 * the contents of a given page of your building instructions.  Once
 * gathered, the contents of the page are translated to graphical representation
 * and presented to the user for editing.
 *
 **************************************************************************/

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QScrollBar>
#include <QPixmap>
#include <QBitmap>
#include <QColor>
#include "lgraphicsscene.h"
#include "commonmenus.h"
#include "callout.h"
#include "lpub.h"
#include "ranges.h"
#include "range.h"
#include "step.h"
#include "meta.h"
#include "color.h"
#include "pagebackgrounditem.h"
#include "numberitem.h"
#include "pageattributetextitem.h"
#include "pageattributepixmapitem.h"
#include "csiitem.h"
#include "calloutbackgrounditem.h"
#include "textitem.h"
#include "rotateiconitem.h"
#include "paths.h"
#include "pagepointer.h"
#include "lgraphicsscene.h"
#include "name.h"

/*
 * We need to draw page every time there is change to the LDraw file.
 *   Changes can come from Menu->dialogs, people editing the file.
 *
 * Gui tracks modified, so whenever things go modified, we need to
 * delete all the GraphicsItems and do a freeranges.h.
 */

void Gui::clearPage(
    LGraphicsView *view,
    LGraphicsScene *scene)
{
  page.freePage();
  page.pli.clear();

  if (view->pageBackgroundItem) {
      delete view->pageBackgroundItem;
      view->pageBackgroundItem = nullptr;
    }
  scene->clear();
}

/*********************************************
 *
 * given a ranges.h for a page, format the
 * entire page.
 *
 ********************************************/

class SubmodelInstanceCount : public NumberPlacementItem
{
  Page *page;

public:
  SubmodelInstanceCount(
      Page                *pageIn,
      NumberPlacementMeta &numberMetaIn,
      const char          *formatIn,
      int                  valueIn,
      QGraphicsItem       *parentIn)    {
    page = pageIn;
    QString toolTip("Times used - right-click to modify");
    setAttributes(SubmodelInstanceCountType,
                  page->relativeType,
                  numberMetaIn,
                  formatIn,
                  valueIn,
                  toolTip,
                  parentIn);
    setData(ObjectId, SubmodelInstanceCountObj);
    setZValue(page->meta.LPub.page.scene.submodelInstanceCount.zValue());
  }
protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

void SubmodelInstanceCount::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QString name = "Submodel Instance Count";

  QAction *fontAction       = commonMenus.fontMenu(menu,name);
  QAction *colorAction      = commonMenus.colorMenu(menu,name);
  QAction *marginAction     = commonMenus.marginMenu(menu,name);
  QAction *placementAction  = commonMenus.placementMenu(menu,name,"You can move this submodel count around.");

  QAction *selectedAction   = menu.exec(event->screenPos());

  Where topOfSteps          = page->topOfSteps();
  Where bottomOfSteps       = page->bottomOfSteps();
  bool  useTop              = parentRelativeType != StepGroupType;

  if (selectedAction == nullptr) {
    return;
  } else if (selectedAction == fontAction) {

      changeFont(topOfSteps,
                 bottomOfSteps,
                &font,1,true,
                 useTop);

    } else if (selectedAction == colorAction) {

      changeColor(topOfSteps,
                  bottomOfSteps,
                 &color,1,true,
                  useTop);

    } else if (selectedAction == marginAction) {

      changeMargins(name + " Margins",
                    topOfSteps,
                    bottomOfSteps,
                   &margin,
                    useTop);

    } else if (selectedAction == placementAction) {
      changePlacement(parentRelativeType,
                      SubmodelInstanceCountType,
                      "Move " + name,
                      topOfSteps,
                      bottomOfSteps,
                      &placement,
                      useTop);
    }
}

void SubmodelInstanceCount::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

      QPointF newPosition;

      Where topOfSteps    = page->topOfSteps();
      Where bottomOfSteps = page->bottomOfSteps();
      bool  useTop        = parentRelativeType != StepGroupType;

      // back annotate the movement of the PLI into the LDraw file.
      newPosition = pos() - position;

      if (newPosition.x() || newPosition.y()) {
          positionChanged = true;

          PlacementData placementData = placement.value();

          placementData.offsets[0] += newPosition.x()/relativeToSize[0];
          placementData.offsets[1] += newPosition.y()/relativeToSize[1];

          placement.setValue(placementData);

          changePlacementOffset(useTop ? topOfSteps:bottomOfSteps,
                                &placement,StepNumberType);
        }
    }
}

/*********************************************************************************
 * addGraphicsPageItems - this function is given the page contents in tree
 * data structure form, with all the CSI and PLI images rendered.  This function
 * then creates Qt graphics view items for each of the components of the page.
 * Simple things like page number, page background, inserts and the like are
 * handled here. For things like callouts, page pointers and step groups, there is a bunch
 * of packing and placing things relative to things that must go on, these
 * operations are handled elsewhere (step.cpp, steps.cpp, callout.cpp, placement.cpp
 * etc.)
 *
 * This function is used for on screen display, printing to PDF and exporting
 * to images per page.
 ********************************************************************************/

int Gui::addGraphicsPageItems(
    Steps          *steps,
    bool            coverPage,
    bool            endOfSubmodel,
    LGraphicsView  *view,
    LGraphicsScene *scene,
    bool            printing)
{

  Page                    *page     = dynamic_cast<Page *>(steps);

  Placement                plPage;
  PlacementHeader         *pageHeader;
  PlacementFooter         *pageFooter;
  PageBackgroundItem      *pageBg;
  PageNumberItem          *pageNumber = nullptr;
  SubmodelInstanceCount   *instanceCount;

  int pW, pH;

  if (printing) {
      pW = view->maximumWidth();
      pH = view->maximumHeight();
    } else {
      // Flip page size per orientation and return size in pixels
      pW = pageSize(page->meta.LPub.page, 0);
      pH = pageSize(page->meta.LPub.page, 1);
    }

//  logDebug() << QString("  DRAW PAGE %3 SIZE PIXELS - WidthPx: %1 x HeightPx: %2 CurPage: %3")
//                .arg(QString::number(pW), QString::number(pH)).arg(stepPageNum);

  pageBg = new PageBackgroundItem(page, pW, pH, exporting());

  view->pageBackgroundItem = pageBg;
  pageBg->setPos(0,0);

  // Set up the placement aspects of the page in the Qt space

  plPage.relativeType = PageType;

  plPage.setSize(pW,pH);
  plPage.margin  = page->meta.LPub.page.margin;
  plPage.loc[XX] = 0;
  plPage.loc[YY] = 0;

  // Set up page header and footer //~~~~~~~~~~~~~~~~

  int which = page->meta.LPub.page.orientation.value() == Landscape ? 1 : 0;
  float _pW = page->meta.LPub.page.size.value(which);
  page->meta.LPub.page.pageHeader.size.setValue(0,_pW);
  page->meta.LPub.page.pageFooter.size.setValue(0,_pW);

  pageHeader = new PlacementHeader(page->meta.LPub.page.pageHeader,pageBg);
  if (pageHeader->placement.value().relativeTo == plPage.relativeType) {
      plPage.appendRelativeTo(pageHeader);
      plPage.placeRelative(pageHeader);

    }
  pageHeader->setPos(pageHeader->loc[XX],pageHeader->loc[YY]);

  pageFooter = new PlacementFooter(page->meta.LPub.page.pageFooter,pageBg);
  if (pageFooter->placement.value().relativeTo == plPage.relativeType) {
      plPage.appendRelativeTo(pageFooter);
      plPage.placeRelative(pageFooter);
    }
  pageFooter->setPos(pageFooter->loc[XX],pageFooter->loc[YY]);

  // Process Cover Page Attriutes

  addCoverPageAttributes(page,pageBg,pageHeader,pageFooter,plPage);

  // Display the page number and instance count

  bool displayPageNumber = page->meta.LPub.page.dpn.value();
  if (displayPageNumber && ! coverPage) {

      // allocate QGraphicsTextItem for page number

      pageNumber =
          new PageNumberItem(
            page,
            page->meta.LPub.page.number,
            "%d",
            stepPageNum,
            pageBg);

      pageNumber->setZValue(page->meta.LPub.page.scene.pageNumber.zValue());
      pageNumber->relativeType = PageNumberType;
      pageNumber->size[XX]     = (int) pageNumber->document()->size().width();
      pageNumber->size[YY]     = (int) pageNumber->document()->size().height();

      PlacementData  placementData;

      placementData = pageNumber->placement.value();

      if (page->meta.LPub.page.togglePnPlacement.value() && ! (stepPageNum & 1)) {
          switch (placementData.placement) {
            case TopLeft:
              placementData.placement = TopRight;
              break;
            case Top:
            case Bottom:
              switch (placementData.justification) {
                case Left:
                  placementData.justification = Right;
                  break;
                case Right:
                  placementData.justification = Left;
                  break;
                default:
                  break;
                }
              break;
            case TopRight:
              placementData.placement = TopLeft;
              break;
            case Left:
              placementData.placement = Right;
              break;
            case Right:
              placementData.placement = Left;
              break;
            case BottomLeft:
              placementData.placement = BottomRight;
              break;
            case BottomRight:
              placementData.placement = BottomLeft;
              break;
            default:
              break;
            }
          pageNumber->placement.setValue(placementData);
        }

      plPage.appendRelativeTo(pageNumber);
      plPage.placeRelative(pageNumber);
      pageNumber->setPos(pageNumber->loc[XX],pageNumber->loc[YY]);

      // if this page contains the last step of the submodel,
      // and instance is > 1 and merge instance count, then display instance

      // allocate QGraphicsTextItem for instance number

      if (endOfSubmodel && page->displayInstanceCount/*page->instances > 1*/) {

          instanceCount = new SubmodelInstanceCount(
                page,
                page->meta.LPub.page.instanceCount,
                "x%d ",
                page->instances,
                pageBg);

          /*
       * To make mousemove always know how to calculate offset, I modified
       * SubmodelInstanceClass to be derived from Placement.  The relativeToSize
       * offset calculation are in Placement.
       *
       * The offset calculation works great, but we end up with a problem
       * SubmodelInstanceCount gets placement from NumberPlacementItem, and
       * placement from Placement.  To work around this, I had to hack (and I mean
       * ugly) SubmodelInstanceCount to Placement.
       */

          if (instanceCount) {
              instanceCount->setSize(int(instanceCount->document()->size().width()),
                                     int(instanceCount->document()->size().height()));
              instanceCount->loc[XX] = 0;
              instanceCount->loc[YY] = 0;
              instanceCount->tbl[0] = 0;
              instanceCount->tbl[1] = 0;

              instanceCount->placement = page->meta.LPub.page.instanceCount.placement;

    //              logDebug() << "TogglePageNumber:" << page->meta.LPub.page.togglePnPlacement.value();
    //              logDebug() << (page->meta.LPub.page.number.number % 2 ?
    //                                 tr("Page %1 is Even").arg(stepPageNum) :
    //                                 tr("Page %1 is Odd").arg(stepPageNum));
    //              logDebug() << "Placement: " << RectNames[page->meta.LPub.page.instanceCount.placement.value().rectPlacement]
    //                         << "(" << page->meta.LPub.page.instanceCount.placement.value().rectPlacement << ")" ;

              PlacementData placementData = instanceCount->placement.value();

              if (placementData.relativeTo == PageNumberType) {

                  if (page->meta.LPub.page.togglePnPlacement.value() &&
                      ! (stepPageNum % 2 /* if page is odd */)) {
                      switch (placementData.rectPlacement){
                      case (TopLeftOutsideCorner):
                          instanceCount->placement.setValue(TopRightOutsideCorner,PageNumberType);
                          break;
                      case (TopLeftOutside):
                          instanceCount->placement.setValue(TopRightOutSide,PageNumberType);
                          break;
                      case (LeftTopOutside):
                          instanceCount->placement.setValue(RightTopOutside,PageNumberType);
                          break;
                      case (LeftOutside):
                          instanceCount->placement.setValue(RightOutside,PageNumberType);
                          break;
                      case (LeftBottomOutside):
                          instanceCount->placement.setValue(RightBottomOutside,PageNumberType);
                          break;
                      case (BottomLeftOutsideCorner):
                          instanceCount->placement.setValue(BottomRightOutsideCorner,PageNumberType);
                          break;
                      case (BottomLeftOutside):
                          instanceCount->placement.setValue(BottomRightOutside,PageNumberType);
                          break;
                      default:
                          instanceCount->placement = page->meta.LPub.page.instanceCount.placement;
                          break;
                      }
                    } else {
                      instanceCount->placement = page->meta.LPub.page.instanceCount.placement;
                    }
                  pageNumber->placeRelative(instanceCount);
                } else {
                  instanceCount->placement.setValue(BottomRightInsideCorner,PageType);
                  plPage.placeRelative(instanceCount);
                }
              instanceCount->setPos(instanceCount->loc[XX],instanceCount->loc[YY]);
            }
        }

      // Process Content Page Attributes - with page number

      addContentPageAttributes(page,pageBg,pageHeader,pageFooter,pageNumber,plPage,false);

    } else {

      // Process Content Page Attributes - without page number and end of submodel

      addContentPageAttributes(page,pageBg,pageHeader,pageFooter,nullptr,plPage,endOfSubmodel);
    }

  /* Create any graphics items in the insert list */

  int nInserts = page->inserts.size();

  if (nInserts) {
      QFileInfo fileInfo, picInfo;
      for (int i = 0; i < nInserts; i++) {
          InsertData insert = page->inserts[i].value();

          switch (insert.type) {
            case InsertData::InsertPicture:
              {
                picInfo.setFile(insert.picName);
                QString filename(picInfo.fileName());

                fileInfo.setFile(QDir::currentPath() + "/" + filename); // relative path

                if (!fileInfo.exists()) {
                  fileInfo.setFile(picInfo.absoluteFilePath());         // insert path
                } else {
                  insert.picName = fileInfo.absoluteFilePath();         // update insert path
                }

                if (fileInfo.exists()) {

                    QPixmap qpixmap;
                    qpixmap.load(insert.picName);
                    InsertPixmapItem *pixmap = new InsertPixmapItem(qpixmap,page->inserts[i],pageBg);
                    pixmap->setZValue(page->meta.LPub.page.scene.insertPicture.zValue());

                    page->addInsertPixmap(pixmap);
                    pixmap->setTransformationMode(Qt::SmoothTransformation);
                    pixmap->setScale(insert.picScale,insert.picScale);

                    PlacementData pld;

                    pld.placement      = TopLeft;
                    pld.justification  = Center;
                    pld.relativeTo     = PageType;
                    pld.preposition    = Inside;
                    pld.offsets[0]     = insert.offsets[0];
                    pld.offsets[1]     = insert.offsets[1];

                    pixmap->placement.setValue(pld);

                    int margin[2] = {0, 0};

                    plPage.placeRelative(pixmap, margin);
                    pixmap->setPos(pixmap->loc[XX],pixmap->loc[YY]);
                    pixmap->relativeToSize[0] = plPage.size[XX];
                    pixmap->relativeToSize[1] = plPage.size[YY];
                } else {
                  emit messageSig(LOG_ERROR, QString("Unable to locate picture %1. Be sure picture file "
                                                     "is located relative to model file or use an absolute path.").arg(filename));
                }
              }
              break;
            case InsertData::InsertText:
              {
                TextItem *text = new TextItem(page->inserts[i],pageBg);
                text->setZValue(page->meta.LPub.page.scene.insertText.zValue());

                PlacementData pld;

                pld.placement     = TopLeft;
                pld.justification = Center;
                pld.relativeTo    = PageType;
                pld.preposition   = Inside;
                pld.offsets[0]    = insert.offsets[0];
                pld.offsets[1]    = insert.offsets[1];

                text->placement.setValue(pld);

                int margin[2] = {0, 0};

                plPage.placeRelative(text, margin);
                text->setPos(text->loc[XX],text->loc[YY]);
                text->relativeToSize[0] = plPage.size[XX];
                text->relativeToSize[1] = plPage.size[YY];
              }
              break;
            case InsertData::InsertArrow:
              break;
            case InsertData::InsertBom:
              {
                Where where(insert.where.modelName, insert.where.lineNumber);
                Where current(ldrawFile.topLevelFile(),0);
                QList<PliPartGroupMeta> bomPartGroups;
                QStringList bomParts;
                QString addLine;
                getBOMParts(current,addLine,bomParts,bomPartGroups);
                page->pli.steps = steps;
                getBOMOccurrence(current);
                page->pli.setParts(bomParts,bomPartGroups,page->meta,true,(boms > 1/*Split BOM Parts*/));
                bomParts.clear();
                page->pli.sizePli(&page->meta,page->relativeType,false);
                page->pli.relativeToSize[0] = plPage.size[XX];
                page->pli.relativeToSize[1] = plPage.size[YY];
              }
              break;
            case InsertData::InsertRotateIcon:
              break;
            }
        }
    }

  // If the page contains a single step then process it here
  if (page->relativeType == SingleStepType && page->list.size()) {
      if (page->list.size()) {
          Range *range = dynamic_cast<Range *>(page->list[0]);
          if (range->relativeType == RangeType) {
              Step *step = dynamic_cast<Step *>(range->list[0]);
              if (step && step->relativeType == StepType) {

                  // populate page pixmaps - when using LDView Single Call

                  if (renderer->useLDViewSCall()){
                      addStepImageGraphics(step);
                    }

                  // set PLI relativeType

                  if (!page->pli.bom)
                      step->pli.relativeType = PartsListType;

                  // if show step number

                  if (! step->onlyChild() && step->showStepNumber) {

                      // add the step number

                      step->stepNumber.sizeit();

                      // if Submodel and Pli relative to StepNumber

                      if (step->placeSubModel &&
                              step->subModel.placement.value().relativeTo == StepNumberType) {

                          // Redirect Pli relative to SubModel

                          if (step->pli.placement.value().relativeTo == StepNumberType) {

                              step->pli.placement.setValue(BottomLeftOutside,SubModelType);
                              step->subModel.appendRelativeTo(&step->pli);
                              step->subModel.placeRelative(&step->pli);
                          }
                      }
                  }

                  // if no step number

                  else {

                      // if Submodel and Pli relative to StepNumber

                      if (step->placeSubModel &&
                          step->subModel.placement.value().relativeTo == StepNumberType) {

                          // Redirect Submodel relative to PageHeader

                          step->subModel.placement.setValue(BottomLeftOutside,PageHeaderType);
                          pageHeader->appendRelativeTo(&step->subModel);
                          pageHeader->placeRelative(&step->subModel);

                          if (step->pli.placement.value().relativeTo == StepNumberType) {

                              // Redirect Pli relative to SubModel

                              step->pli.placement.setValue(BottomLeftOutside,SubModelType);
                              step->subModel.appendRelativeTo(&step->pli);
                              step->subModel.placeRelative(&step->pli);
                          }
                      }

                      // if Pli relative to StepNumber

                      else {

                          // Redirect Pli relative to PageHeader

                          step->pli.placement.setValue(BottomLeftOutside,PageHeaderType);
                          pageHeader->appendRelativeTo(&step->pli);
                          pageHeader->placeRelative(&step->pli);
                      }
                  }

                  // size the callouts

                  for (int i = 0; i < step->list.size(); i++) {
                      step->list[i]->sizeIt();
                  }

                  // add the assembly image to the scene

                  step->csiItem = new CsiItem(
                        step,
                        &page->meta,
                        step->csiPixmap,
                        step->submodelLevel,
                        pageBg,
                        page->relativeType);

                  if (step->csiItem == nullptr) {
                      exit(-1);
                    }
                  step->csiItem->assign(&step->csiPlacement);
                  step->csiItem->boundingSize[XX] = step->csiItem->size[XX];
                  step->csiItem->boundingSize[YY] = step->csiItem->size[YY];

                  // add the SM graphically to the scene

                  if (step->placeSubModel) {
                      step->subModel.addSubModel(step->submodelLevel, pageBg);
                  }

                  // add the PLI graphically to the scene

                  step->pli.addPli(step->submodelLevel, pageBg);

                  // Place the step relative to the page.

                  plPage.relativeTo(step);      // place everything - calculate placement for all page objects

                  // center the csi's bounding box relative to the page
                  // if there is no offset, otherwise place as is

                  PlacementData csiPlacementData = step->csiPlacement.placement.value();
                  if (csiPlacementData.offsets[XX] == 0.0f && csiPlacementData.offsets[YY] == 0.0f) {
                      plPage.placeRelativeBounding(step->csiItem);
                  }

                  // place callouts relative to the csi bounding box

                  for (int i = 0; i < step->list.size(); i++) {

                      if (step->list[i]->relativeType == CalloutType) {
                          Callout *callout = dynamic_cast<Callout *>(step->list[i]);

                          PlacementData placementData = callout->placement.value();

                          if (placementData.relativeTo == CsiType) {
                              step->csiItem->placeRelativeBounding(callout);
                            }
                        } // if callout
                    } // callouts

                  // place the CSI relative to the entire step's box

                  step->csiItem->setPos(step->csiItem->loc[XX],
                                        step->csiItem->loc[YY]);

                  // place CSI annotations //

                  if (step->csiItem->assem->annotation.display.value() &&
                      ! gui->exportingObjects())
                      step->csiItem->placeCsiPartAnnotations();

                  // add the SM relative to the entire step's box

                  if (step->placeSubModel) {
                      step->subModel.setPos(step->subModel.loc[XX],
                                            step->subModel.loc[YY]);
                  }

                  // place the PLI relative to the entire step's box

                  step->pli.setPos(step->pli.loc[XX],
                                   step->pli.loc[YY]);

                  // allocate QGraphicsPixmapItem for rotate icon

                  if (step->placeRotateIcon && page->meta.LPub.rotateIcon.display.value()) {

                      step->rotateIcon.sizeit();
                      RotateIconItem *rotateIcon =
                          new RotateIconItem(
                            step,
                            page->relativeType,
                            page->meta.LPub.rotateIcon,
                            pageBg);
                      rotateIcon->setPos(step->rotateIcon.loc[XX],
                                         step->rotateIcon.loc[YY]);
                      rotateIcon->relativeToSize[0] = step->rotateIcon.relativeToSize[0];
                      rotateIcon->relativeToSize[1] = step->rotateIcon.relativeToSize[1];
                      rotateIcon->setFlag(QGraphicsItem::ItemIsMovable,true);
                      rotateIcon->setZValue(page->meta.LPub.page.scene.rotateIconBackground.zValue());
                    }

                  // allocate QGraphicsTextItem for step number

                  if (! step->onlyChild() || ! step->displayModelOnlyStep()) {
                      StepNumberItem *stepNumber =
                          new StepNumberItem(step,
                                             page->relativeType,
                                             page->meta.LPub.stepNumber,
                                             "%d",
                                             step->stepNumber.number,
                                             pageBg);
                      stepNumber->setZValue(page->meta.LPub.page.scene.stepNumber.zValue());
                      stepNumber->setPos(step->stepNumber.loc[XX],
                                         step->stepNumber.loc[YY]);
                      stepNumber->relativeToSize[0] = step->stepNumber.relativeToSize[0];
                      stepNumber->relativeToSize[1] = step->stepNumber.relativeToSize[1];

                    }

                  // add callout pointer items to the scene

                  for (int i = 0; i < step->list.size(); i++) {

                      // foreach callout

                      Callout *callout = step->list[i];

                      QRect    csiRect(step->csiItem->loc[XX]-callout->loc[XX],
                                       step->csiItem->loc[YY]-callout->loc[YY],
                                       step->csiItem->size[XX],
                                       step->csiItem->size[YY]);

                      // add the callout's graphics items to the scene
                      callout->addGraphicsItems(0,0,csiRect,pageBg,true);

                      // foreach pointer
                      //   add the pointer to the graphics scene
                      for (int i = 0; i < callout->pointerList.size(); i++) {
                          Pointer *pointer = callout->pointerList[i];
                          callout->addGraphicsPointerItem(pointer,callout->underpinnings);
                        }
                    }

                  // add page pointer base and pointers to the scene

                  for (auto i : page->pagePointers.keys()) {
                      PagePointer *pp = page->pagePointers[i];

                      // override the default location
                      if (pp->placement.value().placement == Left ||
                          pp->placement.value().placement == Right)
                          pp->loc[YY] = pp->loc[YY] - (plPage.size[YY]/2);
                      else
                      if (pp->placement.value().placement == Top ||
                          pp->placement.value().placement == Bottom)
                          pp->loc[XX] = pp->loc[XX] - (plPage.size[XX]/2);

                      // size the pagePointer origin (hidden base rectangle)
                      pp->sizeIt();

                      // add the pagePointer origin (hidden base rectangle) to the graphics scene
                      pp->addGraphicsItems(0,0,pageBg,true); // set true to make movable

                      //   add the pagePointer pointers to the graphics scene
                      for (int i = 0; i < pp->pointerList.size(); i++) {
                          Pointer *pointer = pp->pointerList[i];
                          pp->addGraphicsPointerItem(pointer);
                        }
                    }

                  // Place the Bill of Materials on the page along with single step

                  if (page->pli.tsize()) {
                      if (page->pli.bom) {
                          page->pli.relativeType = BomType;
                          page->pli.addPli(0,pageBg);
                          page->pli.setFlag(QGraphicsItem::ItemIsSelectable,true);
                          page->pli.setFlag(QGraphicsItem::ItemIsMovable,true);

                          PlacementData pld;

                          pld = page->pli.pliMeta.placement.value();

                          page->pli.placement.setValue(pld);
                          if (pld.relativeTo == PageType) {
                              plPage.placeRelative(page->pli.background);
                            } else {
                              step->csiItem->placeRelative(page->pli.background);
                            }
                          page->pli.loc[XX] = page->pli.background->loc[XX];
                          page->pli.loc[YY] = page->pli.background->loc[YY];

                          page->pli.setPos(page->pli.loc[XX],page->pli.loc[YY]);
                        }
                    }
                }
            }
        }

    } else {

      // qDebug() << "List relative type: " << RelNames[range->relativeType];
      // We've got a page that contains step groups, so add it

      // LDView generate multistep pixamps
      if (renderer->useLDViewSCall() &&
          page->list.size()) {
          for (int i = 0; i < page->list.size(); i++){
              Range *range = dynamic_cast<Range *>(page->list[i]);
              for (int j = 0; j < range->list.size(); j++){
                  if (range->relativeType == RangeType) {
                      Step *step = dynamic_cast<Step *>(range->list[j]);
                      if (step && step->relativeType == StepType){
                          // Load images and set size
                          addStepImageGraphics(step);
                      } // 1.4 validate if relativeType is StepType - to add image, check for Callout
                  } // 1.3 validate if relativeType is RangeType - to cast as Step
              } // 1.2 for each list-item (Step) within a Range...=>list[AbstractRangeElement]->StepType
          } // 1.1 for each list-item (Range) within a Page...=>list[AbstractStepsElement]->RangeType
      } // 1.0 Page

      PlacementData data = page->meta.LPub.multiStep.placement.value();
      page->placement.setValue(data);

      // place all the steps in the group relative to each other, including
      // any callouts placed relative to steps

      page->sizeIt();             // size multi-step

      plPage.relativeToSg(page);  // place callouts and page pointers relative to PAGE
      plPage.placeRelative(page); // place multi-step relative to the page

      page->relativeToSg(page);   // compute bounding box of step group and callouts
                                  // placed relative to it.

      plPage.placeRelativeBounding(page); // center multi-step in page's bounding box

      page->relativeToSg(page);           // place callouts relative to MULTI_STEP

      page->addGraphicsItems(0,0,pageBg);

      // add page pointers to the scene

      for (auto i : page->pagePointers.keys()) {
          PagePointer *pp = page->pagePointers[i];

          // override the default location
          if (pp->placement.value().placement == Left ||
              pp->placement.value().placement == Right)
              pp->loc[YY] = pp->loc[YY] - (plPage.size[YY]/2);
          else
          if (pp->placement.value().placement == Top ||
              pp->placement.value().placement == Bottom)
              pp->loc[XX] = pp->loc[XX] - (plPage.size[XX]/2);

          // size the pagePointer origin (hidden page rectangle)
          pp->sizeIt();

          // add the pagePointer origin (page rectangle) to the graphics scene
          pp->addGraphicsItems(0,0,pageBg,true); // set true to make movable

          //   add the pagePointer pointers to the graphics scene
          for (int i = 0; i < pp->pointerList.size(); i++) {
              Pointer *pointer = pp->pointerList[i];
              pp->addGraphicsPointerItem(pointer);
            }
        }

      // Place the Bill of materials on the page along with step group?????

      if (page->pli.tsize()) {
          if (page->pli.bom) {
              page->pli.relativeType = BomType;
              page->pli.addPli(0,pageBg);
              page->pli.setFlag(QGraphicsItem::ItemIsSelectable,true);
              page->pli.setFlag(QGraphicsItem::ItemIsMovable,true);

              PlacementData pld;
              pld = page->pli.pliMeta.placement.value();
              page->pli.placement.setValue(pld);
              if (pld.relativeTo == PageType) {
                  plPage.placeRelative(page->pli.background);
                } else {
                  page->placeRelative(page->pli.background);
                }
              page->pli.loc[XX] = page->pli.background->loc[XX];
              page->pli.loc[YY] = page->pli.background->loc[YY];
            }
          page->pli.setPos(page->pli.loc[XX],page->pli.loc[YY]);
        }
    }

  if ( ! printing) {

      if (pageBg->background.value().type != BackgroundData::BgTransparent){

          QGraphicsDropShadowEffect *bodyShadow = new QGraphicsDropShadowEffect;
          bodyShadow->setBlurRadius(9.0);
          bodyShadow->setColor(QColor(0, 0, 0, 160));
          bodyShadow->setOffset(4.0);

          pageBg->setGraphicsEffect(bodyShadow);
      }

      view->horizontalScrollBar()->setRange(0,pW);
      view->verticalScrollBar()->setRange(  0,pH);

    }

  scene->addItem(pageBg);

  addPliPartGroupsToScene(page, scene);

  view->setSceneRect(pageBg->sceneBoundingRect());

  QRectF pageRect = QRectF(0,0,pW,pH);
  if (printing) {
      view->fitInView(pageRect);
    }
  else
  if (view->fitMode == FitWidth) {
      view->fitWidth(pageRect);
    }
  else
  if (view->fitMode == FitVisible) {
      view->fitVisible(pageRect);
    }

  page->relativeType = SingleStepType;
  statusBarMsg("");
  return 0;
}

/*
 * Add step image graphics
 * This function recurses the step's model to add images.
 * Call only if using LDView Single Call (useLDViewsCall=true)
 */
int Gui::addStepImageGraphics(Step *step) {
  int retVal = 0;
  step->csiPixmap.load(step->pngName);
  step->csiPlacement.size[0] = step->csiPixmap.width();
  step->csiPlacement.size[1] = step->csiPixmap.height();
  // process callout's step(s) image(s)
  for (int k = 0; k < step->list.size(); k++) {
      if (step->list[k]->relativeType == CalloutType) {
          Callout *callout = dynamic_cast<Callout *>(step->list[k]);
          for (int l = 0; l < callout->list.size(); l++){
              Range *range = dynamic_cast<Range *>(callout->list[l]);
              for (int m = 0; m < range->list.size(); m++){
                  if (range->relativeType == RangeType) {
                      Step *step = dynamic_cast<Step *>(range->list[m]);
                      if (step && step->relativeType == StepType){
                          addStepImageGraphics(step);
                      } // 1.6 validate if Step relativeType is StepType - to add image, check for Callout
                  } // 1.5 validate if Range relativeType is RangeType - to cast as Step
              } // 1.4 for each Step list-item within a Range...=>list[AbstractRangeElement]->StepType
          } // 1.3 for each Range list-item within a Callout...=>list[AbstractStepsElement]->RangeType
      } // 1.2 validate if relativeType is CalloutType - to cast as Callout...
  } // 1.1 for each Callout list-item within a Step...=>list[Steps]->CalloutType
  return retVal;
}

int Gui::addPliPartGroupsToScene(
        Page           *page,
        LGraphicsScene *scene)
{
    // add Pli part group to scene
    QHash<QString, PliPart*> pliParts;
    PliPart *part;
    QString key;

    if (page->list.size()){
        // Single Step
        if (page->relativeType == SingleStepType) {
            if (page->list.size()) {
                Range *range = dynamic_cast<Range *>(page->list[0]);
                if (range->relativeType == RangeType) {
                    Step *step = dynamic_cast<Step *>(range->list[0]);
                    if (step && step->relativeType == StepType) {
                        if (step->pli.pliMeta.enablePliPartGroup.value() &&
                            step->pli.pliMeta.show.value() &&
                            step->pli.tsize()) {
                            step->pli.getParts(pliParts);
                            if (pliParts.size()) {
                                foreach(key,pliParts.keys()) {
                                    part = pliParts[key];
                                    part->addPartGroupToScene(scene);
                                }
                            }
                        }
                    }
                }
            }
        }
        // Step Group
        else
        {
            for (int i = 0; i < page->list.size(); i++) {
                Range *range = dynamic_cast<Range *>(page->list[i]);
                for (int j = 0; j < range->list.size(); j++) {
                    if (range->relativeType == RangeType) {
                        Step *step = dynamic_cast<Step *>(range->list[j]);
                        if (step && step->relativeType == StepType) {
                            if (step->pli.pliMeta.enablePliPartGroup.value() &&
                                step->pli.pliMeta.show.value() &&
                                step->pli.tsize()) {
                                step->pli.getParts(pliParts);
                                if (pliParts.size()) {
                                    foreach(key,pliParts.keys()) {
                                        part = pliParts[key];
                                        part->addPartGroupToScene(scene);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // BOM
    if (page->pli.tsize()) {
        if (page->pli.pliMeta.enablePliPartGroup.value() &&
            page->pli.bom) {
            page->pli.getParts(pliParts);
            if (pliParts.size()) {
                foreach(key,pliParts.keys()) {
                    part = pliParts[key];
                    part->addPartGroupToScene(scene);
                }
            }
        }
    }
    return 0;
}

int Gui::addContentPageAttributes(
    Page                *page,
    PageBackgroundItem  *pageBg,
    PlacementHeader     *pageHeader,
    PlacementFooter     *pageFooter,
    PageNumberItem      *pageNumber,
    Placement           &plPage,
    bool                 endOfSubmodel)
{
  // Content Page Initializations and Allocations...
  if (! page->coverPage)
    {
      // Initializations ...

      PageAttributeTextItem   *url       = new PageAttributeTextItem(page,page->meta.LPub.page.url,pageBg);
      PageAttributeTextItem   *email     = new PageAttributeTextItem(page,page->meta.LPub.page.email,pageBg);
      PageAttributeTextItem   *copyright = new PageAttributeTextItem(page,page->meta.LPub.page.copyright,pageBg);
      PageAttributeTextItem   *author    = new PageAttributeTextItem(page,page->meta.LPub.page.author,pageBg);
      bool displayPageNumber = page->meta.LPub.page.dpn.value();

      //  Content Page URL,
      bool displayURL       = page->meta.LPub.page.url.display.value();
      if (displayURL) {
          url->size[XX]     = (int) url->document()->size().width();
          url->size[YY]     = (int) url->document()->size().height();
        } else {
          delete url;
          url               = nullptr;
        }
      //  Content Page Email
      bool displayEmail       = page->meta.LPub.page.email.display.value();
      if (displayEmail){
          email->size[XX]     = (int) email->document()->size().width();
          email->size[YY]     = (int) email->document()->size().height();
        }else {
          delete email;
          email               = nullptr;
        }
      //  Content Page Copyright
      bool displayCopyright       = page->meta.LPub.page.copyright.display.value();
      if (displayCopyright){
          copyright->size[XX]     = (int) copyright->document()->size().width();
          copyright->size[YY]     = (int) copyright->document()->size().height();
        }else {
          delete copyright;
          copyright               = nullptr;
        }
      //  Content Page Author
      bool displayAuthor       = page->meta.LPub.page.author.display.value();
      if (displayAuthor) {
          author->size[XX]     = (int) author->document()->size().width();
          author->size[YY]     = (int) author->document()->size().height();
        }else {
          delete author;
          author               = nullptr;
        }

      // Allocations...

      //  Content Page URL (Header/Footer) //~~~~~~~~~~~~~~~~
      if (displayURL) {
          PlacementData pld = url->placement.value();
          if (pld.relativeTo == PageType) {
              plPage.appendRelativeTo(url);
              plPage.placeRelative(url);
            } else if (pld.relativeTo == PageHeaderType) {
              pageHeader->appendRelativeTo(url);
              pageHeader->placeRelative(url);
            } else if (pld.relativeTo == PageFooterType) {
              pageFooter->appendRelativeTo(url);
              pageFooter->placeRelative(url);
            } else if (displayPageNumber && pld.relativeTo == PageNumberType) {
              pageNumber->appendRelativeTo(url);
              pageNumber->placeRelative(url);
            } else if (displayEmail && pld.relativeTo == PageEmailType) {
              email->appendRelativeTo(url);
              email->placeRelative(url);
            } else if (displayCopyright && pld.relativeTo == PageCopyrightType) {
              copyright->appendRelativeTo(url);
              copyright->placeRelative(url);
            } else if (displayAuthor && pld.relativeTo == PageAuthorType) {
              author->appendRelativeTo(url);
              author->placeRelative(url);
            } else {
              url->placement.setValue(TopLeftInsideCorner,PageType);
              plPage.appendRelativeTo(url);
              plPage.placeRelative(url);
            }
          url->setPos(url->loc[XX],url->loc[YY]);
        }

      //  Content Page Email (Header/Footer) //~~~~~~~~~~~~~~~~
      if (displayEmail) {
          PlacementData pld = email->placement.value();
          if (pld.relativeTo == PageType) {
              plPage.appendRelativeTo(email);
              plPage.placeRelative(email);
            } else if (pld.relativeTo == PageHeaderType) {
              pageHeader->appendRelativeTo(email);
              pageHeader->placeRelative(email);
            } else if (pld.relativeTo == PageFooterType) {
              pageFooter->appendRelativeTo(email);
              pageFooter->placeRelative(email);
            } else if (displayPageNumber && pld.relativeTo == PageNumberType) {
              pageNumber->appendRelativeTo(email);
              pageNumber->placeRelative(email);
            } else if (displayURL && pld.relativeTo == PageURLType) {
              url->appendRelativeTo(email);
              url->placeRelative(email);
            } else if (displayCopyright && pld.relativeTo == PageCopyrightType) {
              copyright->appendRelativeTo(email);
              copyright->placeRelative(email);
            } else if (displayAuthor && pld.relativeTo == PageAuthorType) {
              author->appendRelativeTo(email);
              author->placeRelative(email);
            } else {
              email->placement.setValue(TopRightInsideCorner,PageType);
              plPage.appendRelativeTo(email);
              plPage.placeRelative(email);
            }
          email->setPos(email->loc[XX],email->loc[YY]);
        }

      //  Content Page Copyright (Header/Footer) //~~~~~~~~~~~~~~~~
      if (displayCopyright) {
          PlacementData pld = copyright->placement.value();
          if (pld.relativeTo == PageType) {
              plPage.appendRelativeTo(copyright);
              plPage.placeRelative(copyright);
            } else if (pld.relativeTo == PageHeaderType) {
              pageHeader->appendRelativeTo(copyright);
              pageHeader->placeRelative(copyright);
            } else if (pld.relativeTo == PageFooterType) {
              pageFooter->appendRelativeTo(copyright);
              pageFooter->placeRelative(copyright);
            } else if (displayPageNumber && pld.relativeTo == PageNumberType) {
              pageNumber->appendRelativeTo(copyright);
              pageNumber->placeRelative(copyright);
            } else if (displayURL && pld.relativeTo == PageURLType) {
              url->appendRelativeTo(copyright);
              url->placeRelative(copyright);
            } else if (displayEmail && pld.relativeTo == PageEmailType) {
              email->appendRelativeTo(copyright);
              email->placeRelative(copyright);
            } else if (displayAuthor && pld.relativeTo == PageAuthorType) {
              author->appendRelativeTo(copyright);
              author->placeRelative(copyright);
            } else {
              copyright->placement.setValue(BottomLeftInsideCorner,PageType);
              plPage.appendRelativeTo(copyright);
              plPage.placeRelative(copyright);
            }
          copyright->setPos(copyright->loc[XX],copyright->loc[YY]);
        }

      //  Content Page Author (Header/Footer) //~~~~~~~~~~~~~~~~
      if (displayAuthor) {
          PlacementData pld = author->placement.value();
          if (pld.relativeTo == PageType) {
              plPage.appendRelativeTo(author);
              plPage.placeRelative(author);
            } else if (pld.relativeTo == PageHeaderType) {
              pageHeader->appendRelativeTo(author);
              pageHeader->placeRelative(author);
            } else if (pld.relativeTo == PageFooterType) {
              pageFooter->appendRelativeTo(author);
              pageFooter->placeRelative(author);
            } else if (displayPageNumber && pld.relativeTo == PageNumberType) {
              pageNumber->appendRelativeTo(author);
              pageNumber->placeRelative(author);
            } else if (displayURL && pld.relativeTo == PageURLType) {
              url->appendRelativeTo(author);
              url->placeRelative(author);
            } else if (displayEmail && pld.relativeTo == PageEmailType) {
              email->appendRelativeTo(author);
              email->placeRelative(author);
            } else if (displayCopyright && pld.relativeTo == PageCopyrightType) {
              copyright->appendRelativeTo(author);
              copyright->placeRelative(author);
            } else if (displayPageNumber) {
              author->placement.setValue(LeftBottomOutside,PageNumberType);
              pageNumber->appendRelativeTo(author);
              pageNumber->placeRelative(author);
            } else {
              author->placement.setValue(BottomRightInsideCorner,PageType);
              plPage.appendRelativeTo(author);
              plPage.placeRelative(author);
            }
          author->setPos(author->loc[XX],author->loc[YY]);
        }

      // Place insance count if end of submodel

      if (endOfSubmodel && page->displayInstanceCount/*page->instances > 1*/) {

          SubmodelInstanceCount   *instanceCount;

          instanceCount = new SubmodelInstanceCount(
                page,
                page->meta.LPub.page.instanceCount,
                "x%d ",
                page->instances,
                pageBg);

          if (instanceCount) {
              instanceCount->setSize(int(instanceCount->document()->size().width()),
                                     int(instanceCount->document()->size().height()));
              instanceCount->loc[XX] = 0;
              instanceCount->loc[YY] = 0;
              instanceCount->tbl[0] = 0;
              instanceCount->tbl[1] = 0;

              instanceCount->placement = page->meta.LPub.page.instanceCount.placement;

              if (displayAuthor){
                  PlacementData pld = author->placement.value();
                  if ((pld.rectPlacement == TopLeftOutsideCorner     ||
                       pld.rectPlacement == TopLeftOutside           ||
                       pld.rectPlacement == TopOutside               ||
                       pld.rectPlacement == LeftTopOutside           ||
                       pld.rectPlacement == LeftOutside              ||
                       pld.rectPlacement == LeftBottomOutside       ) &&
                     ((pld.relativeTo    == PageNumberType          )||
                      (pld.rectPlacement == BottomRightInsideCorner  &&
                       pld.relativeTo    == PageType)))
                    {
                      instanceCount->placement.setValue(TopOutside,PageAuthorType);
                      author->appendRelativeTo(instanceCount);
                      author->placeRelative(instanceCount);

                    }
                }
              if (displayEmail){
                  PlacementData pld = email->placement.value();
                  if ((pld.rectPlacement == TopLeftOutsideCorner     ||
                       pld.rectPlacement == TopLeftOutside           ||
                       pld.rectPlacement == TopOutside               ||
                       pld.rectPlacement == LeftTopOutside           ||
                       pld.rectPlacement == LeftOutside              ||
                       pld.rectPlacement == LeftBottomOutside       ) &&
                     ((pld.relativeTo    == PageNumberType          )||
                      (pld.rectPlacement == BottomRightInsideCorner  &&
                       pld.relativeTo    == PageType)))
                    {
                      instanceCount->placement.setValue(TopOutside,PageEmailType);
                      email->appendRelativeTo(instanceCount);
                      email->placeRelative(instanceCount);
                    }
                }
              if (displayURL){
                  PlacementData pld = url->placement.value();
                  if ((pld.rectPlacement == TopLeftOutsideCorner     ||
                       pld.rectPlacement == TopLeftOutside           ||
                       pld.rectPlacement == TopOutside               ||
                       pld.rectPlacement == LeftTopOutside           ||
                       pld.rectPlacement == LeftOutside              ||
                       pld.rectPlacement == LeftBottomOutside       ) &&
                     ((pld.relativeTo    == PageNumberType          )||
                      (pld.rectPlacement == BottomRightInsideCorner  &&
                       pld.relativeTo    == PageType)))
                    {
                      instanceCount->placement.setValue(TopOutside,PageURLType);
                      url->appendRelativeTo(instanceCount);
                      url->placeRelative(instanceCount);
                    }
                }
              if (displayCopyright){
                  PlacementData pld = copyright->placement.value();
                  if ((pld.rectPlacement == TopLeftOutsideCorner     ||
                       pld.rectPlacement == TopLeftOutside           ||
                       pld.rectPlacement == TopOutside               ||
                       pld.rectPlacement == LeftTopOutside           ||
                       pld.rectPlacement == LeftOutside              ||
                       pld.rectPlacement == LeftBottomOutside       ) &&
                     ((pld.relativeTo    == PageNumberType          )||
                      (pld.rectPlacement == BottomRightInsideCorner  &&
                       pld.relativeTo    == PageType)))
                    {
                      instanceCount->placement.setValue(TopOutside,PageCopyrightType);
                      copyright->appendRelativeTo(instanceCount);
                      copyright->placeRelative(instanceCount);
                    }
                }
              else
                {
                  instanceCount->placement.setValue(BottomRightInsideCorner,PageType);
                  plPage.appendRelativeTo(instanceCount);
                  plPage.placeRelative(instanceCount);
                }
            }

          instanceCount->setPos(instanceCount->loc[XX],instanceCount->loc[YY]);
        }
    }
  return 0;
}

int Gui::addCoverPageAttributes(
    Page                *page,
    PageBackgroundItem  *pageBg,
    PlacementHeader     *pageHeader,
    PlacementFooter     *pageFooter,
    Placement           &plPage)
{
  // Front Cover Page Initializations and Allocations...
  if (page->coverPage && page->frontCover) {

      // Initializations...
      PageAttributeTextItem   *titleFront            = new PageAttributeTextItem(page,page->meta.LPub.page.titleFront,pageBg);
      PageAttributeTextItem   *modelNameFront        = new PageAttributeTextItem(page,page->meta.LPub.page.modelName,pageBg);
      PageAttributeTextItem   *authorFront           = new PageAttributeTextItem(page,page->meta.LPub.page.authorFront,pageBg);
      PageAttributeTextItem   *partsFront           = new PageAttributeTextItem(page,page->meta.LPub.page.parts,pageBg);
      PageAttributeTextItem   *modelDescFront        = new PageAttributeTextItem(page,page->meta.LPub.page.modelDesc,pageBg);
      PageAttributeTextItem   *publishDescFront      = new PageAttributeTextItem(page,page->meta.LPub.page.publishDesc,pageBg);
      PageAttributePixmapItem *pixmapLogoFront;
      PageAttributePixmapItem *pixmapCoverImageFront;
      //PageAttributeTextItem   *categoryFront       = new PageAttributeTextItem(page,page->meta.LPub.page.category,pageBg);

      // Front Cover Title
      bool displayTitleFront         = page->meta.LPub.page.titleFront.display.value();
      bool breakTitleFrontRelativeTo = false;
      PlacementData titleFrontPld;
      if (displayTitleFront) {
          titleFront->size[XX]       = (int) titleFront->document()->size().width();
          titleFront->size[YY]       = (int) titleFront->document()->size().height();
          titleFrontPld = titleFront->placement.value();
          breakTitleFrontRelativeTo  = titleFrontPld.relativeTo != PageType;
      } else {
          delete titleFront;
          titleFront                 = nullptr;
        }

      // Front Cover Model Name
      bool displayModelNameFront       = page->meta.LPub.page.modelName.display.value();
      PlacementData modelNameFrontPld;
      if (displayModelNameFront){
          modelNameFront->size[XX]     = (int) modelNameFront->document()->size().width();
          modelNameFront->size[YY]     = (int) modelNameFront->document()->size().height();
          modelNameFrontPld = modelNameFront->placement.value();
        } else {
          delete modelNameFront;
          modelNameFront               = nullptr;
        }

       // Front Cover Author
      bool displayAuthorFront         = page->meta.LPub.page.authorFront.display.value();
      bool breakAuthorFrontRelativeTo = false;
      PlacementData authorFrontPld;
      if (displayAuthorFront) {
          authorFront->size[XX]       = (int) authorFront->document()->size().width();
          authorFront->size[YY]       = (int) authorFront->document()->size().height();
          authorFrontPld = authorFront->placement.value();
          breakAuthorFrontRelativeTo  = authorFrontPld.relativeTo != PageTitleType;
      } else {
          delete authorFront;
          authorFront                 = nullptr;
        }

      // Front Page Parts Count
      bool displayPartsFront         = page->meta.LPub.page.parts.display.value();
      bool breakPartsFrontRelativeTo = false;
      PlacementData PartsFrontPld;
      if (displayPartsFront) {
          partsFront->size[XX]       = (int) partsFront->document()->size().width();
          partsFront->size[YY]       = (int) partsFront->document()->size().height();
          PartsFrontPld = partsFront->placement.value();
          breakPartsFrontRelativeTo  = PartsFrontPld.relativeTo != PageAuthorType;
      } else {
          delete  partsFront;
          partsFront                 = nullptr;
        }

      // Front Page Model Description,
      bool displayModelDescFront         = page->meta.LPub.page.modelDesc.display.value();
      bool breakModelDescFrontRelativeTo = false;
      PlacementData modelDescFrontPld;
      if (displayModelDescFront) {
          modelDescFront->size[XX]       = (int) modelDescFront->document()->size().width();
          modelDescFront->size[YY]       = (int) modelDescFront->document()->size().height();
          modelDescFrontPld = modelDescFront->placement.value();
          breakModelDescFrontRelativeTo  = modelDescFrontPld.relativeTo != PagePartsType;
      } else {
          delete modelDescFront;
          modelDescFront                 = nullptr;
        }

      // Front Page Publish Description,
      bool displayPublishDescFront       = page->meta.LPub.page.publishDesc.display.value();
      PlacementData publishDescFrontPld;
      if (displayPublishDescFront) {
          publishDescFront->size[XX]     = (int) publishDescFront->document()->size().width();
          publishDescFront->size[YY]     = (int) publishDescFront->document()->size().height();
          publishDescFrontPld = publishDescFront->placement.value();
      } else {
          delete publishDescFront;
          publishDescFront               = nullptr;
        }

      // Front Page Category,
      /* bool displayCategoryFront      = page->meta.LPub.page.category.display.value();
      bool breakCategoryFrontRelativeTo = false;
      PlacementData categoryFrontPld;
      if (displayCategoryFront) {
          categoryFront->size[XX]       = (int) categoryFront->document()->size().width();
          categoryFront->size[YY]       = (int) categoryFront->document()->size().height();

          categoryFrontPld = categoryFront->placement.value();
          breakCategoryFrontRelativeTo  = categoryFrontPld.relativeTo != PagePartsType;
      } else {
          delete  categoryFront;
          categoryFront                 = nullptr;
        } */

      // Front Cover Document Logo
      bool displayDocumentLogoFront  = page->meta.LPub.page.documentLogoFront.display.value();

      // Front Cover Image
      bool displayCoverImage  = page->meta.LPub.page.coverImage.display.value();

      // Allocations...

      // Title (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayTitleFront) {
          plPage.appendRelativeTo(titleFront);
          plPage.placeRelative(titleFront);
          titleFront->setPos(titleFront->loc[XX],titleFront->loc[YY]);
        }

      // ModelName (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayModelNameFront) {
          if (breakTitleFrontRelativeTo && modelNameFrontPld.relativeTo == PageTitleType) {
              modelNameFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(modelNameFront);
              plPage.placeRelative(modelNameFront);
            } else if (displayTitleFront && modelNameFrontPld.relativeTo == PageTitleType) {
              titleFront->appendRelativeTo(modelNameFront);
              titleFront->placeRelative(modelNameFront);
            } else {
              modelNameFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(modelNameFront);
              plPage.placeRelative(modelNameFront);
            }
          modelNameFront->setPos(modelNameFront->loc[XX],modelNameFront->loc[YY]);
        }

      // Author (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayAuthorFront) {
          if (breakTitleFrontRelativeTo && authorFrontPld.relativeTo == PageTitleType) {
              authorFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(authorFront);
              plPage.placeRelative(authorFront);
            } else if (displayTitleFront && authorFrontPld.relativeTo == PageTitleType) {
              titleFront->appendRelativeTo(authorFront);
              titleFront->placeRelative(authorFront);
            } else {
              authorFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(authorFront);
              plPage.placeRelative(authorFront);
            }
          authorFront->setPos(authorFront->loc[XX],authorFront->loc[YY]);
        }

      // Parts (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayPartsFront) {
          if (breakAuthorFrontRelativeTo && PartsFrontPld.relativeTo == PageAuthorType) {
              partsFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(partsFront);
              plPage.placeRelative(partsFront);
            } else if (displayAuthorFront && PartsFrontPld.relativeTo == PageAuthorType) {
              authorFront->appendRelativeTo(partsFront);
              authorFront->placeRelative(partsFront);
            } else {
              partsFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(partsFront);
              plPage.placeRelative(partsFront);
            }
          partsFront->setPos(partsFront->loc[XX],partsFront->loc[YY]);
        }

      // ModelDesc (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayModelDescFront) {
          if ((breakPartsFrontRelativeTo && modelDescFrontPld.relativeTo == PagePartsType)    /* ||
              (breakCategoryFrontRelativeTo && modelDescFrontPld.relativeTo == PageCategoryType) */) {
              modelDescFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(modelDescFront);
              plPage.placeRelative(modelDescFront);
          } else if (displayPartsFront && modelDescFrontPld.relativeTo == PagePartsType) {
              partsFront->appendRelativeTo(modelDescFront);
              partsFront->placeRelative(modelDescFront);
          /*
          } else if (displayCategoryFront && modelDescFrontPld.relativeTo == PageCategoryType) {
              partsFront->appendRelativeTo(modelDescFront);
              partsFront->placeRelative(modelDescFront);
          */
          } else {
              modelDescFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(modelDescFront);
              plPage.placeRelative(modelDescFront);
          }
          modelDescFront->setPos(modelDescFront->loc[XX],modelDescFront->loc[YY]);
        }

      // PublishDesc (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayPublishDescFront) {
          if (breakModelDescFrontRelativeTo && publishDescFrontPld.relativeTo == PageModelDescType) {
              publishDescFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(publishDescFront);
              plPage.placeRelative(publishDescFront);
            } else if (displayModelDescFront && publishDescFrontPld.relativeTo == PageModelDescType) {
              modelDescFront->appendRelativeTo(publishDescFront);
              modelDescFront->placeRelative(publishDescFront);
            } else {
              publishDescFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(publishDescFront);
              plPage.placeRelative(publishDescFront);
            }
          publishDescFront->setPos(publishDescFront->loc[XX],publishDescFront->loc[YY]);
        }

      // Category (Front Cover) //~~~~~~~~~~~~~~~~
      /* if (displayCategoryFront) {
          if (breakPartsFrontRelativeTo && categoryFrontPld.relativeTo == PagePartsType) {
              categoryFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(categoryFront);
              plPage.placeRelative(categoryFront);
          } else if (displayPartsFront && categoryFrontPld.relativeTo == PagePartsType) {
              partsFront->appendRelativeTo(categoryFront);
              partsFront->placeRelative(categoryFront);
          } else {
              categoryFront->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(categoryFront);
              plPage.placeRelative(categoryFront);
          }
          categoryFront->setPos(categoryFront->loc[XX],categoryFront->loc[YY]);
      } */

      // DocumentLogo (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayDocumentLogoFront) {
          QFileInfo fileInfo;
          QString file   = page->meta.LPub.page.documentLogoFront.file.value();
          qreal picScale = page->meta.LPub.page.documentLogoFront.picScale.value();
          fileInfo.setFile(file);
          if (fileInfo.exists()) {
              QPixmap qpixmap;
              qpixmap.load(file);
              pixmapLogoFront
                  = new PageAttributePixmapItem(
                    page,
                    qpixmap,
                    page->meta.LPub.page.documentLogoFront,
                    pageBg);
              page->addPageAttributePixmap(pixmapLogoFront);
              pixmapLogoFront->setTransformationMode(Qt::SmoothTransformation);
              pixmapLogoFront->setScale(picScale,picScale);
              pixmapLogoFront->setZValue(page->meta.LPub.page.scene.pageAttributePixmap.zValue());
              int margin[2] = {0, 0};
              PlacementData pld = pixmapLogoFront->placement.value();
              if (pld.relativeTo == PageHeaderType) {
                  pageHeader->placeRelative(pixmapLogoFront, margin);
                  pixmapLogoFront->relativeToSize[0] = pageHeader->size[XX];
                  pixmapLogoFront->relativeToSize[1] = pageHeader->size[YY];
                } else if (pld.relativeTo == PageFooterType) {
                  pageFooter->placeRelative(pixmapLogoFront, margin);
                  pixmapLogoFront->relativeToSize[0] = pageFooter->size[XX];
                  pixmapLogoFront->relativeToSize[1] = pageFooter->size[YY];
                } else {
                  pixmapLogoFront->placement.setValue(TopLeftInsideCorner,PageType);
                  plPage.placeRelative(pixmapLogoFront, margin);
                  pixmapLogoFront->relativeToSize[0] = plPage.size[XX];
                  pixmapLogoFront->relativeToSize[1] = plPage.size[YY];
                }
              pixmapLogoFront->setPos(pixmapLogoFront->loc[XX],pixmapLogoFront->loc[YY]);
            }
        }

      // CoverImage (Front Cover) //~~~~~~~~~~~~~~~~
      if (displayCoverImage) {
          QFileInfo fileInfo;
          QString file = page->meta.LPub.page.coverImage.file.value();
          qreal picScale   = page->meta.LPub.page.coverImage.picScale.value();
          fileInfo.setFile(file);
          if (fileInfo.exists()) {
              QPixmap qpixmap;
              qpixmap.load(file);
              pixmapCoverImageFront
                  = new PageAttributePixmapItem(
                    page,
                    qpixmap,
                    page->meta.LPub.page.coverImage,
                    pageBg);
              page->addPageAttributePixmap(pixmapCoverImageFront);
              pixmapCoverImageFront->setTransformationMode(Qt::SmoothTransformation);
              pixmapCoverImageFront->setScale(picScale,picScale);
              pixmapCoverImageFront->setZValue(page->meta.LPub.page.scene.pageAttributePixmap.zValue());
              int margin[2] = {0, 0};
              plPage.placeRelative(pixmapCoverImageFront, margin);
              pixmapCoverImageFront->setPos(pixmapCoverImageFront->loc[XX],pixmapCoverImageFront->loc[YY]);
              pixmapCoverImageFront->relativeToSize[0] = plPage.size[XX];
              pixmapCoverImageFront->relativeToSize[1] = plPage.size[YY];

            }
        }
    }

  // Back Cover Page Initializations and Allocations...
  if (page->coverPage && page->backCover){

      // Initializations...
      PageAttributeTextItem   *titleBack          = new PageAttributeTextItem(page,page->meta.LPub.page.titleBack,pageBg);
      PageAttributeTextItem   *authorBack         = new PageAttributeTextItem(page,page->meta.LPub.page.authorBack,pageBg);
      PageAttributeTextItem   *copyrightBack      = new PageAttributeTextItem(page,page->meta.LPub.page.copyrightBack,pageBg);
      PageAttributeTextItem   *urlBack            = new PageAttributeTextItem(page,page->meta.LPub.page.urlBack,pageBg);
      PageAttributeTextItem   *emailBack          = new PageAttributeTextItem(page,page->meta.LPub.page.emailBack,pageBg);
      PageAttributeTextItem   *disclaimerBack     = new PageAttributeTextItem(page,page->meta.LPub.page.disclaimer,pageBg);
      PageAttributeTextItem   *plugBack           = new PageAttributeTextItem(page,page->meta.LPub.page.plug,pageBg);;
      PageAttributePixmapItem *pixmapLogoBack;
      PageAttributePixmapItem *pixmapPlugImageBack;

      // Back Cover Title
      bool displayTitleBack = page->meta.LPub.page.titleBack.display.value();
      bool breakTitleBackRelativeTo = false;
      PlacementData titleBackPld;
      if (displayTitleBack) {
          titleBack->size[XX]     = (int) titleBack->document()->size().width();
          titleBack->size[YY]     = (int) titleBack->document()->size().height();

          titleBackPld = titleBack->placement.value();
          breakTitleBackRelativeTo = titleBackPld.relativeTo != PageType;
      } else {
          delete titleBack;
          titleBack               = nullptr;
        }

      // Back Cover Author
      bool displayAuthorBack = page->meta.LPub.page.authorBack.display.value();
      bool breakAuthorBackRelativeTo = false;
      PlacementData authorBackPld;
      if (displayAuthorBack) {
          authorBack->size[XX]     = (int) authorBack->document()->size().width();
          authorBack->size[YY]     = (int) authorBack->document()->size().height();
          authorBackPld = authorBack->placement.value();
          breakAuthorBackRelativeTo = authorBackPld.relativeTo != PageTitleType;
      } else {
          delete authorBack;
          authorBack               = nullptr;
        }

      // Back Cover Copyright
      bool displayCopyrightBack = page->meta.LPub.page.copyrightBack.display.value();
      bool breakCopyrightBackRelativeTo = false;
      PlacementData copyrightBackPld;
      if (displayCopyrightBack) {
          copyrightBack->size[XX]     = (int) copyrightBack->document()->size().width();
          copyrightBack->size[YY]     = (int) copyrightBack->document()->size().height();
          copyrightBackPld = copyrightBack->placement.value();
          breakCopyrightBackRelativeTo = copyrightBackPld.relativeTo != PageAuthorType;
      } else {
          delete copyrightBack;
          copyrightBack               = nullptr;
        }

      // Back Cover URL
      bool displayUrlBack = page->meta.LPub.page.urlBack.display.value();
      bool breakURLBackRelativeTo = false;
      PlacementData urlBackPld;
      if (displayUrlBack) {
          urlBack->size[XX]     = (int) urlBack->document()->size().width();
          urlBack->size[YY]     = (int) urlBack->document()->size().height();
          urlBackPld = urlBack->placement.value();
          breakURLBackRelativeTo = urlBackPld.relativeTo != PageCopyrightType;
      } else {
          delete urlBack;
          urlBack               = nullptr;
        }

      // Back Cover Email
      bool displayEmailBack = page->meta.LPub.page.emailBack.display.value();
      bool breakEmailBackRelativeTo = false;
      PlacementData emailBackPld;
      if (displayEmailBack) {
          emailBack->size[XX]     = (int) emailBack->document()->size().width();
          emailBack->size[YY]     = (int) emailBack->document()->size().height();
          emailBackPld = emailBack->placement.value();
          breakEmailBackRelativeTo = emailBackPld.relativeTo != PageURLType;
      } else {
          delete emailBack;
          emailBack               = nullptr;
        }

      // Back Cover Disclaimer
      bool displayDisclaimerBack = page->meta.LPub.page.disclaimer.display.value();
      bool breakDisclaimerBackRelativeTo = false;
      PlacementData disclaimerBackPld;
      if (displayDisclaimerBack) {
          disclaimerBack->size[XX]     = (int) disclaimerBack->document()->size().width();
          disclaimerBack->size[YY]     = (int) disclaimerBack->document()->size().height();
          disclaimerBackPld = disclaimerBack->placement.value();
          breakDisclaimerBackRelativeTo = disclaimerBackPld.relativeTo != PageEmailType;
      } else {
          delete disclaimerBack;
          disclaimerBack               = nullptr;
        }

      // Back Cover Plug
      bool displayPlugBack = page->meta.LPub.page.plug.display.value();
      PlacementData plugBackPld;
      if (displayPlugBack) {
          plugBack->size[XX]     = (int) plugBack->document()->size().width();
          plugBack->size[YY]     = (int) plugBack->document()->size().height();
          plugBackPld = plugBack->placement.value();
      } else {
          delete plugBack;
          plugBack               = nullptr;
        }

      // Back Cover DocumentLogo
      bool displayDocumentLogoBack = page->meta.LPub.page.documentLogoBack.display.value();

      // Back Cover Plug Image
      bool displayPlugImageBack    = page->meta.LPub.page.plugImage.display.value();

      // Allocations...

      // Title (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayTitleBack) {
          plPage.appendRelativeTo(titleBack);
          plPage.placeRelative(titleBack);
          titleBack->setPos(titleBack->loc[XX],titleBack->loc[YY]);
        }

      // Author (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayAuthorBack) {
          if (breakTitleBackRelativeTo && authorBackPld.relativeTo == PageTitleType) {
              authorBack->placement.setValue(CenterCenter,PageType);
              plPage.appendRelativeTo(authorBack);
              plPage.placeRelative(authorBack);
            } else if (displayTitleBack && authorBackPld.relativeTo == PageTitleType) {
              titleBack->appendRelativeTo(authorBack);
              titleBack->placeRelative(authorBack);
            } else {
              authorBack->placement.setValue(CenterCenter,PageType);
              plPage.appendRelativeTo(authorBack);
              plPage.placeRelative(authorBack);
            }
          authorBack->setPos(authorBack->loc[XX],authorBack->loc[YY]);
        }

      // Copyright (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayCopyrightBack) {
          if (breakAuthorBackRelativeTo && copyrightBackPld.relativeTo == PageAuthorType) {
              copyrightBack->placement.setValue(LeftInside,PageType);
              plPage.appendRelativeTo(copyrightBack);
              plPage.placeRelative(copyrightBack);
            } else if (displayAuthorBack && copyrightBackPld.relativeTo == PageAuthorType) {
              authorBack->appendRelativeTo(copyrightBack);
              authorBack->placeRelative(copyrightBack);
            } else {
              copyrightBack->placement.setValue(LeftInside,PageType);
              plPage.appendRelativeTo(copyrightBack);
              plPage.placeRelative(copyrightBack);
            }
          copyrightBack->setPos(copyrightBack->loc[XX],copyrightBack->loc[YY]);
        }

      // Url (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayUrlBack) {
          urlBackPld = urlBack->placement.value();
          if (breakCopyrightBackRelativeTo && urlBackPld.relativeTo == PageCopyrightType) {
              urlBack->placement.setValue(LeftInside,PageType);
              plPage.appendRelativeTo(urlBack);
              plPage.placeRelative(urlBack);
            } else if (displayCopyrightBack && urlBackPld.relativeTo == PageCopyrightType) {
              copyrightBack->appendRelativeTo(urlBack);
              copyrightBack->placeRelative(urlBack);
            } else {
              urlBack->placement.setValue(LeftInside,PageType);
              plPage.appendRelativeTo(urlBack);
              plPage.placeRelative(urlBack);
            }
          urlBack->setPos(urlBack->loc[XX],urlBack->loc[YY]);
        }

      // Email (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayEmailBack) {
          emailBackPld = emailBack->placement.value();
          if (breakURLBackRelativeTo && emailBackPld.relativeTo == PageURLType) {
              emailBack->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(emailBack);
              plPage.placeRelative(emailBack);
            } else if (displayUrlBack && emailBackPld.relativeTo == PageURLType) {
              urlBack->appendRelativeTo(emailBack);
              urlBack->placeRelative(emailBack);
            } else {
              emailBack->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(emailBack);
              plPage.placeRelative(emailBack);
            }
          emailBack->setPos(emailBack->loc[XX],emailBack->loc[YY]);
        }

      // Disclaimer (Back Cover)//~~~~~~~~~~~~~~~~
      if (displayDisclaimerBack) {
          if (breakEmailBackRelativeTo && disclaimerBackPld.relativeTo == PageEmailType) {
              disclaimerBack->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(disclaimerBack);
              plPage.placeRelative(disclaimerBack);
            } else if (displayEmailBack && disclaimerBackPld.relativeTo == PageEmailType) {
              emailBack->appendRelativeTo(disclaimerBack);
              emailBack->placeRelative(disclaimerBack);
            } else {
              disclaimerBack->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(disclaimerBack);
              plPage.placeRelative(disclaimerBack);
            }
          disclaimerBack->setPos(disclaimerBack->loc[XX],disclaimerBack->loc[YY]);
        }

      // Plug (Back Cover)//~~~~~~~~~~~~~~~~
      if (displayPlugBack) {
          if (breakDisclaimerBackRelativeTo && plugBackPld.relativeTo == PageDisclaimerType) {
              plugBack->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(plugBack);
              plPage.placeRelative(plugBack);
            } else if (displayDisclaimerBack && plugBackPld.relativeTo == PageDisclaimerType) {
              disclaimerBack->appendRelativeTo(plugBack);
              disclaimerBack->placeRelative(plugBack);
            } else {
              plugBack->placement.setValue(RightInside,PageType);
              plPage.appendRelativeTo(plugBack);
              plPage.placeRelative(plugBack);
            }
          plugBack->setPos(plugBack->loc[XX],plugBack->loc[YY]);
        }

      // DocumentLogoBack (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayDocumentLogoBack) {
          QFileInfo fileInfo;
          QString file = page->meta.LPub.page.documentLogoBack.file.value();
          qreal picScale   = page->meta.LPub.page.documentLogoBack.picScale.value();
          fileInfo.setFile(file);
          if (fileInfo.exists()) {
              QPixmap qpixmap;
              qpixmap.load(file);
              pixmapLogoBack =
                  new PageAttributePixmapItem(
                    page,
                    qpixmap,
                    page->meta.LPub.page.documentLogoBack,
                    pageBg);
              page->addPageAttributePixmap(pixmapLogoBack);
              pixmapLogoBack->setTransformationMode(Qt::SmoothTransformation);
              pixmapLogoBack->setScale(picScale,picScale);
              pixmapLogoBack->setZValue(page->meta.LPub.page.scene.pageAttributePixmap.zValue());
              int margin[2] = {0, 0};
              PlacementData pld = pixmapLogoBack->placement.value();
              if (pld.relativeTo == PageHeaderType) {
                  pageHeader->placeRelative(pixmapLogoBack, margin);
                  pixmapLogoBack->relativeToSize[0] = pageHeader->size[XX];
                  pixmapLogoBack->relativeToSize[1] = pageHeader->size[YY];
                } else if (pld.relativeTo == PageFooterType) {
                  pageFooter->placeRelative(pixmapLogoBack, margin);
                  pixmapLogoBack->relativeToSize[0] = pageFooter->size[XX];
                  pixmapLogoBack->relativeToSize[1] = pageFooter->size[YY];
                } else {
                  pixmapLogoBack->placement.setValue(TopLeftInsideCorner,PageType);
                  plPage.placeRelative(pixmapLogoBack, margin);
                  pixmapLogoBack->relativeToSize[0] = plPage.size[XX];
                  pixmapLogoBack->relativeToSize[1] = plPage.size[YY];
                }
              pixmapLogoBack->setPos(pixmapLogoBack->loc[XX],pixmapLogoBack->loc[YY]);
            }
        }

      // PlugImage (Back Cover) //~~~~~~~~~~~~~~~~
      if (displayPlugImageBack) {
          QFileInfo fileInfo;
          QString file     = page->meta.LPub.page.plugImage.file.value();
          qreal picScale   = page->meta.LPub.page.plugImage.picScale.value();
          fileInfo.setFile(file);
          if (fileInfo.exists()) {
              QPixmap qpixmap;
              qpixmap.load(file);
              pixmapPlugImageBack =
                  new PageAttributePixmapItem(
                    page,
                    qpixmap,
                    page->meta.LPub.page.plugImage,
                    pageBg);
              page->addPageAttributePixmap(pixmapPlugImageBack);;
              pixmapPlugImageBack->setTransformationMode(Qt::SmoothTransformation);
              pixmapPlugImageBack->setScale(picScale,picScale);
              pixmapPlugImageBack->setZValue(page->meta.LPub.page.scene.pageAttributePixmap.zValue());
              int margin[2] = {0, 0};
              PlacementData pld = pixmapPlugImageBack->placement.value();
              if (displayPlugBack && pld.relativeTo == PagePlugType) {
                  plugBack->placeRelative(pixmapPlugImageBack, margin);
                  pixmapPlugImageBack->relativeToSize[0] = plugBack->size[XX];
                  pixmapPlugImageBack->relativeToSize[1] = plugBack->size[YY];
                } else if (pld.relativeTo == PageFooterType) {
                  pageFooter->placeRelative(pixmapPlugImageBack, margin);
                  pixmapPlugImageBack->relativeToSize[0] = pageFooter->size[XX];
                  pixmapPlugImageBack->relativeToSize[1] = pageFooter->size[YY];
                } else {
                  pixmapPlugImageBack->placement.setValue(BottomInside,PageType);
                  plPage.placeRelative(pixmapPlugImageBack, margin);
                  pixmapPlugImageBack->relativeToSize[0] = plPage.size[XX];
                  pixmapPlugImageBack->relativeToSize[1] = plPage.size[YY];
                }
              pixmapPlugImageBack->setPos(pixmapPlugImageBack->loc[XX],pixmapPlugImageBack->loc[YY]);
            }
        }
    }
  return 0;
}
