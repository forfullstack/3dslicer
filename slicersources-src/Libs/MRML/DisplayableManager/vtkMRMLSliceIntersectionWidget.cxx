/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMRMLSliceIntersectionWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMRMLSliceIntersectionWidget.h"

#include "vtkMRMLAbstractSliceViewDisplayableManager.h"
#include "vtkMRMLApplicationLogic.h"
#include "vtkMRMLCrosshairDisplayableManager.h"
#include "vtkMRMLCrosshairNode.h"
#include "vtkMRMLInteractionEventData.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSliceCompositeNode.h"
#include "vtkMRMLSliceIntersectionRepresentation2D.h"
#include "vtkMRMLSliceLayerLogic.h"
#include "vtkMRMLVolumeNode.h"

#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkEvent.h"
#include "vtkGeneralTransform.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"
#include "vtkWidgetEvent.h"

#include <deque>

vtkStandardNewMacro(vtkMRMLSliceIntersectionWidget);

static void getTimeBuf(char* tmbuf)
{
    time_t now;
    const char* fmt;
#define DAY_FMT "%d"

    fmt = DAY_FMT " %b %H:%M:%S";
    time(&now);
    localtime(&now);

    strftime(tmbuf, 32, fmt, localtime(&now));
}

static void outputLog(const char* buf)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");

    char tmbuf[100];
    getTimeBuf(tmbuf);

    if (fp)
    {
        fprintf(fp, "%s: %s\n", tmbuf, buf);
        fflush(fp);
        fclose(fp);
    }
}

static void outputLogBool(const char* buf, bool value)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");

    char tmbuf[100];
    getTimeBuf(tmbuf);

    if (fp)
    {
        fprintf(fp, "%s: %s %d\n", tmbuf, buf, value);
        fflush(fp);
        fclose(fp);
    }
}

static void outputLogLong(const char* buf, unsigned long val)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");
    char tmbuf[100];
    getTimeBuf(tmbuf);
    if (fp)
    {
        fprintf(fp, "%s: %s val: %u\n", tmbuf, buf, val);
        fflush(fp);

        fclose(fp);
    }
}

static void outputLogDouble(char* buf, double *val)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");
    char tmbuf[100];
    getTimeBuf(tmbuf);
    if (fp)
    {
        fprintf(fp, "%s: %s val: %f %f %f\n", tmbuf, buf, val[0], val[1], val[2]);
        fflush(fp);

        fclose(fp);
    }
}

static void outputLogBounds(char* buf, double *val)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");
    char tmbuf[100];
    getTimeBuf(tmbuf);
    if (fp)
    {
      fprintf(fp, "%s: %s val: %f %f %f\n", tmbuf, buf, val[0], val[1], val[2], val[3], val[4], val[5]);
      fflush(fp);

      fclose(fp);
    }
}

static void outputLogIntDouble(char* buf, int *val)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");
    char tmbuf[100];
    getTimeBuf(tmbuf);
    if (fp)
    {
        fprintf(fp, "%s: %s val: %d %d \n", tmbuf, buf, val[0], val[1]);
        fflush(fp);

        fclose(fp);
    }
}

static void outputLogDoubleOne(char* buf, double val)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");
    char tmbuf[100];
    getTimeBuf(tmbuf);
    if (fp)
    {
        fprintf(fp, "%s: %s val: %f\n", tmbuf, buf, val);
        fflush(fp);

        fclose(fp);
    }
}

static void outputLogDoubleTwo(char* buf, double val1, double val2)
{
    FILE* fp = fopen("E:\\W\\test.log", "at");
    char tmbuf[100];
    getTimeBuf(tmbuf);
    if (fp)
    {
        fprintf(fp, "%s: %s val: %f %f\n", tmbuf, buf, val1, val2);
        fflush(fp);

        fclose(fp);
    }
}

//----------------------------------------------------------------------------------
vtkMRMLSliceIntersectionWidget::vtkMRMLSliceIntersectionWidget()
{
  this->StartEventPosition[0] = 0.0;
  this->StartEventPosition[1] = 0.0;

  this->PreviousRotationAngleRad = 0.0;
  this->CurrentRotationAngleRad = 0.0;

  this->PreviousEventPosition[0] = 0;
  this->PreviousEventPosition[1] = 0;

  this->StartRotationCenter[0] = 0.0;
  this->StartRotationCenter[1] = 0.0;

  this->StartRotationCenter_RAS[0] = 0.0;
  this->StartRotationCenter_RAS[1] = 0.0;
  this->StartRotationCenter_RAS[2] = 0.0;
  this->StartRotationCenter_RAS[3] = 1.0; // to allow easy homogeneous tranformations

  this->StartActionFOV[0] = 0.;
  this->StartActionFOV[1] = 0.;
  this->StartActionFOV[2] = 0.;
  this->VolumeScalarRange[0] = 0;
  this->VolumeScalarRange[1] = 0;

  this->LastForegroundOpacity = 0.;
  this->LastLabelOpacity = 0.;
  this->StartActionSegmentationDisplayNode = nullptr;

  this->ModifierKeyPressedSinceLastMouseButtonRelease = true;

  this->TouchRotationThreshold = 10.0;
  this->TouchZoomThreshold = 0.1;
  this->TouchTranslationThreshold = 25.0;

  this->TotalTouchRotation = 0.0;
  this->TouchRotateEnabled = false;
  this->TotalTouchZoom = 0.0;
  this->TouchZoomEnabled = false;
  this->TotalTouchTranslation = 0.0;
  this->TouchTranslationEnabled = false;

  // Threshold to check if mouse is on the MPR line
  this->TouchMoveThreshold = 3.0;
  this->DragXPrepared = false;
  this->DragYPrepared = false;
  // this->DragXEnabled = false;
  // this->DragYEnabled = false;

  this->SliceLogicsModifiedCommand->SetClientData(this);
  this->SliceLogicsModifiedCommand->SetCallback(vtkMRMLSliceIntersectionWidget::SliceLogicsModifiedCallback);

  this->ActionsEnabled = ActionAll;
  this->UpdateInteractionEventMapping();
}

//----------------------------------------------------------------------------------
vtkMRMLSliceIntersectionWidget::~vtkMRMLSliceIntersectionWidget()
{
  this->SetMRMLApplicationLogic(nullptr);
}

//----------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::UpdateInteractionEventMapping()
{
  this->EventTranslators.clear();

  this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseMoveEvent, vtkEvent::NoModifier, WidgetEventSetCursorPosition);
  if (this->GetActionEnabled(ActionRotateSliceIntersection))
    {
    // Touch gesture slice rotate
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::StartRotateEvent, vtkEvent::AnyModifier, WidgetEventTouchGestureStart);
    this->SetEventTranslation(WidgetStateTouchGesture, vtkCommand::RotateEvent, vtkEvent::AnyModifier, WidgetEventTouchRotateSliceIntersection);
    this->SetEventTranslation(WidgetStateTouchGesture, vtkCommand::EndRotateEvent, vtkEvent::AnyModifier, WidgetEventTouchGestureEnd);

    // this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::LeftButtonPressEvent,
    //   vtkEvent::AltModifier+vtkEvent::ControlModifier, WidgetStateRotate, WidgetEventRotateStart, WidgetEventRotateEnd);
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::LeftButtonPressEvent,
      vtkEvent::NoModifier, WidgetStateRotate, WidgetEventRotateStart, WidgetEventRotateEnd);
    }
  if (this->GetActionEnabled(ActionTranslateSliceIntersection))
    {
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::LeftButtonPressEvent,
      vtkEvent::AltModifier+vtkEvent::ShiftModifier, WidgetStateTranslate, WidgetEventTranslateStart, WidgetEventTranslateEnd);
    }
  if (this->GetActionEnabled(ActionSetCrosshairPosition))
    {
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseMoveEvent, vtkEvent::ShiftModifier, WidgetEventSetCrosshairPosition);

    // this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseMoveEvent, vtkEvent::NoModifier, WidgetEventSetCursorPosition);
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, WidgetStateMPR, WidgetEventMPRStart, WidgetEventMPREnd);
    }
  if (this->GetActionEnabled(ActionBlend))
    {
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::LeftButtonPressEvent,
      vtkEvent::ControlModifier, WidgetStateBlend, WidgetEventBlendStart, WidgetEventBlendEnd);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "g", WidgetEventToggleLabelOpacity);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "t", WidgetEventToggleForegroundOpacity);
    }
  if (this->GetActionEnabled(ActionBrowseSlice))
    {
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "Right", WidgetEventIncrementSlice);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "Left", WidgetEventDecrementSlice);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "Up", WidgetEventIncrementSlice);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "Down", WidgetEventDecrementSlice);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "f", WidgetEventIncrementSlice);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "b", WidgetEventDecrementSlice);
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseWheelForwardEvent, vtkEvent::NoModifier, WidgetEventIncrementSlice);
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseWheelBackwardEvent, vtkEvent::NoModifier, WidgetEventDecrementSlice);
    }
  if (this->GetActionEnabled(ActionShowSlice))
    {
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "v", WidgetEventToggleSliceVisibility);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::AnyModifier, 0, 0, "V", WidgetEventToggleAllSlicesVisibility);
    }
  if (this->GetActionEnabled(ActionSelectVolume))
    {
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::AnyModifier, 0, 0, "bracketleft", WidgetEventShowPreviousBackgroundVolume);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::AnyModifier, 0, 0, "bracketright", WidgetEventShowNextBackgroundVolume);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::AnyModifier, 0, 0, "braceleft", WidgetEventShowPreviousForegroundVolume);
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::AnyModifier, 0, 0, "braceright", WidgetEventShowNextForegroundVolume);
    }
  if (this->GetActionEnabled(ActionTranslate) && this->GetActionEnabled(ActionZoom))
    {
    this->SetKeyboardEventTranslation(WidgetStateIdle, vtkEvent::NoModifier, 0, 0, "r", WidgetEventResetFieldOfView);
    }
  if (this->GetActionEnabled(ActionZoom))
    {
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::RightButtonPressEvent, vtkEvent::NoModifier,
      WidgetStateZoomSlice, WidgetEventZoomSliceStart, WidgetEventZoomSliceEnd);
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseWheelForwardEvent, vtkEvent::ControlModifier, WidgetEventZoomOutSlice);
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::MouseWheelBackwardEvent, vtkEvent::ControlModifier, WidgetEventZoomInSlice);
    // Touch slice zoom
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::StartPinchEvent, vtkEvent::AnyModifier, WidgetEventTouchGestureStart);
    this->SetEventTranslation(WidgetStateTouchGesture, vtkCommand::PinchEvent, vtkEvent::AnyModifier, WidgetEventTouchZoomSlice);
    this->SetEventTranslation(WidgetStateTouchGesture, vtkCommand::EndPinchEvent, vtkEvent::AnyModifier, WidgetEventTouchGestureEnd);
    }

  if (this->GetActionEnabled(ActionTranslate))
    {
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::LeftButtonPressEvent, vtkEvent::ShiftModifier,
      WidgetStateTranslateSlice, WidgetEventTranslateSliceStart, WidgetEventTranslateSliceEnd);
    this->SetEventTranslationClickAndDrag(WidgetStateIdle, vtkCommand::MiddleButtonPressEvent, vtkEvent::NoModifier,
      WidgetStateTranslateSlice, WidgetEventTranslateSliceStart, WidgetEventTranslateSliceEnd);
    // Touch slice translate
    this->SetEventTranslation(WidgetStateIdle, vtkCommand::StartPanEvent, vtkEvent::AnyModifier, WidgetEventTouchGestureStart);
    this->SetEventTranslation(WidgetStateTouchGesture, vtkCommand::PanEvent, vtkEvent::AnyModifier, WidgetEventTouchTranslateSlice);
    this->SetEventTranslation(WidgetStateTouchGesture, vtkCommand::EndPanEvent, vtkEvent::AnyModifier, WidgetEventTouchGestureEnd);
    }
}

//----------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::CreateDefaultRepresentation()
{
  if (this->WidgetRep)
    {
    // already created
    return;
    }
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  if (!mrmlAppLogic)
    {
    vtkWarningMacro("vtkMRMLSliceIntersectionWidget::CreateDefaultRepresentation failed: application logic invalid");
    return;
    }
  vtkNew<vtkMRMLSliceIntersectionRepresentation2D> newRep;
  newRep->SetMRMLApplicationLogic(mrmlAppLogic);
  newRep->SetSliceNode(this->SliceNode);
  this->WidgetRep = newRep;
}

//-----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::CanProcessInteractionEvent(vtkMRMLInteractionEventData* eventData, double &distance2)
{
  if (!this->SliceLogic)
    {
    return false;
    }

  if (eventData->GetType() == vtkCommand::LeftButtonReleaseEvent
    || eventData->GetType() == vtkCommand::MiddleButtonReleaseEvent
    || eventData->GetType() == vtkCommand::RightButtonReleaseEvent)
    {
    this->ModifierKeyPressedSinceLastMouseButtonRelease = false;
    }
  if (eventData->GetType() == vtkCommand::LeaveEvent)
    {
    // We cannot capture keypress events until the user clicks in the view
    // so when we are outside then we should assume that modifier
    // is not just "stuck".
    this->ModifierKeyPressedSinceLastMouseButtonRelease = true;
    }
  if (eventData->GetType() == vtkCommand::KeyPressEvent)
    {
    if (eventData->GetKeySym().find("Shift") != std::string::npos)
      {
      this->ModifierKeyPressedSinceLastMouseButtonRelease = true;
      }
    }

  unsigned long widgetEvent = this->TranslateInteractionEventToWidgetEvent(eventData);
  if (widgetEvent == WidgetEventNone)
    {
    return false;
    }
  if (!this->GetRepresentation())
    {
    return false;
    }

  // If we are currently dragging a point then we interact everywhere
    if (this->WidgetState == WidgetStateTranslate || this->WidgetState == WidgetStateRotate || this->WidgetState == WidgetStateBlend || this->WidgetState == WidgetStateMPR || this->WidgetState == WidgetStateTranslateSlice || this->WidgetState == WidgetStateZoomSlice || this->WidgetState == WidgetStateMPR)
    {
    distance2 = 0.0;
    return true;
    }

  distance2 = 1e10; // we can process this event but we let more specific widgets to claim it (if they are closer)
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessInteractionEvent(vtkMRMLInteractionEventData* eventData)
{
  if (!this->SliceLogic)
    {
    return false;
    }

  unsigned long widgetEvent = this->TranslateInteractionEventToWidgetEvent(eventData);

  bool processedEvent = true;

  switch (widgetEvent)
    {
    case WidgetEventMouseMove:
      // click-and-dragging the mouse cursor
      processedEvent = this->ProcessMouseMove(eventData);
      break;
    case  WidgetEventTouchGestureStart:
      this->ProcessTouchGestureStart(eventData);
      break;
    case WidgetEventTouchGestureEnd:
      this->ProcessTouchGestureEnd(eventData);
      break;
    case WidgetEventTouchRotateSliceIntersection:
      this->ProcessTouchRotate(eventData);
      break;
    case WidgetEventTouchZoomSlice:
      this->ProcessTouchZoom(eventData);
      break;
    case WidgetEventTouchTranslateSlice:
      this->ProcessTouchTranslate(eventData);
      break;
    case WidgetEventTranslateStart:
      this->SetWidgetState(WidgetStateTranslate);
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      processedEvent = this->ProcessStartMouseDrag(eventData);
      break;
    case WidgetEventTranslateEnd:
      this->UpdateCrosshair(eventData);
      processedEvent = this->ProcessEndMouseDrag(eventData);
      break;
    case WidgetEventRotateStart:
      if (this->DragXPrepared || this->DragYPrepared)
      {
        this->SetWidgetState(WidgetStateMPR);
        this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
        processedEvent = this->ProcessStartMouseDrag(eventData);
      }
      else
      {
        this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
        processedEvent = this->ProcessRotateStart(eventData);
      }
      break;
    case WidgetEventRotateEnd:
      if (this->DragXPrepared || this->DragYPrepared)
      {
        this->UpdateCrosshairMPR(eventData);
        // this->DragXPrepared = false;
        // this->DragYPrepared = false;
      }
      processedEvent = this->ProcessEndMouseDrag(eventData);
      break;
    case WidgetEventTranslateSliceStart:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->SliceLogic->StartSliceNodeInteraction(vtkMRMLSliceNode::XYZOriginFlag);
      this->SetWidgetState(WidgetStateTranslateSlice);
      processedEvent = this->ProcessStartMouseDrag(eventData);
      break;
    case WidgetEventTranslateSliceEnd:
      processedEvent = this->ProcessEndMouseDrag(eventData);
      this->SliceLogic->EndSliceNodeInteraction();
      break;
    case WidgetEventZoomSliceStart:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->SetWidgetState(WidgetStateZoomSlice);
      this->SliceLogic->StartSliceNodeInteraction(vtkMRMLSliceNode::FieldOfViewFlag);
      this->SliceLogic->GetSliceNode()->GetFieldOfView(this->StartActionFOV);
      processedEvent = this->ProcessStartMouseDrag(eventData);
      break;
    case WidgetEventZoomSliceEnd:
      processedEvent = this->ProcessEndMouseDrag(eventData);
      this->SliceLogic->EndSliceNodeInteraction();
      break;
    case WidgetEventBlendStart:
      {
      this->SetWidgetState(WidgetStateBlend);
      vtkMRMLSliceCompositeNode *sliceCompositeNode = this->SliceLogic->GetSliceCompositeNode();
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->LastForegroundOpacity = sliceCompositeNode->GetForegroundOpacity();
      this->LastLabelOpacity = this->GetLabelOpacity();
      this->StartActionSegmentationDisplayNode = this->GetVisibleSegmentationDisplayNode();
      processedEvent = this->ProcessStartMouseDrag(eventData);
      }
      break;
    case WidgetEventBlendEnd:
      processedEvent = this->ProcessEndMouseDrag(eventData);
      break;
    case WidgetEventSetCrosshairPosition:
      processedEvent = this->ProcessSetCrosshair(eventData);
      break;
    case WidgetEventSetCursorPosition:
      processedEvent = this->ProcessMPR(eventData);
      break;
    case WidgetEventMPRStart:
      if (this->DragXPrepared || this->DragYPrepared)
      {
        this->SetWidgetState(WidgetStateMPR);
        this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
        processedEvent = this->ProcessStartMouseDrag(eventData);
      }
      break;
    case WidgetEventMPREnd:
      this->UpdateCrosshairMPR(eventData);
      // this->DragXPrepared = false;
      // this->DragYPrepared = false;

      processedEvent = this->ProcessEndMouseDrag(eventData);
      break;
    case WidgetEventToggleLabelOpacity:
      {
      this->StartActionSegmentationDisplayNode = nullptr;
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->StartActionSegmentationDisplayNode = nullptr;      double opacity = this->GetLabelOpacity();
      if (opacity != 0.0)
        {
        this->LastLabelOpacity = opacity;
        this->SetLabelOpacity(0.0);
        }
      else
        {
        this->SetLabelOpacity(this->LastLabelOpacity);
        }
      }
      break;
    case WidgetEventToggleForegroundOpacity:
      {
      vtkMRMLSliceCompositeNode *sliceCompositeNode = this->SliceLogic->GetSliceCompositeNode();
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      double opacity = sliceCompositeNode->GetForegroundOpacity();
      if (opacity != 0.0)
        {
        this->LastForegroundOpacity = opacity;
        sliceCompositeNode->SetForegroundOpacity(0.0);
        }
      else
        {
        sliceCompositeNode->SetForegroundOpacity(this->LastForegroundOpacity);
        }
      }
      break;
    case WidgetEventIncrementSlice:
      this->IncrementSlice();
      break;
    case WidgetEventDecrementSlice:
      this->DecrementSlice();
      break;
    case WidgetEventToggleSliceVisibility:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->GetSliceNode()->SetSliceVisible(!this->GetSliceNode()->GetSliceVisible());
      break;
    case WidgetEventToggleAllSlicesVisibility:
      // TODO: need to set all slices visible
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->GetSliceNode()->SetSliceVisible(!this->GetSliceNode()->GetSliceVisible());
      break;
    case WidgetEventResetFieldOfView:
      {
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->SliceLogic->StartSliceNodeInteraction(vtkMRMLSliceNode::ResetFieldOfViewFlag);
      this->SliceLogic->FitSliceToAll();
      this->GetSliceNode()->UpdateMatrices();
      this->SliceLogic->EndSliceNodeInteraction();
      }
      break;
    case WidgetEventZoomInSlice:
      this->ScaleZoom(1.2, eventData);
      break;
    case WidgetEventZoomOutSlice:
      this->ScaleZoom(0.8, eventData);
      break;
    break;
    case WidgetEventShowPreviousBackgroundVolume:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->CycleVolumeLayer(LayerBackground, -1);
      break;
    case WidgetEventShowNextBackgroundVolume:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->CycleVolumeLayer(LayerBackground, 1);
      break;
    case WidgetEventShowPreviousForegroundVolume:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->CycleVolumeLayer(LayerForeground, -1);
      break;
    case WidgetEventShowNextForegroundVolume:
      this->SliceLogic->GetMRMLScene()->SaveStateForUndo();
      this->CycleVolumeLayer(LayerForeground, 1);
      break;


    default:
      processedEvent = false;
    }

  return processedEvent;
}

//-------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::Leave(vtkMRMLInteractionEventData* eventData)
{
  this->Superclass::Leave(eventData);
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessMouseMove(vtkMRMLInteractionEventData* eventData)
{
  if (!this->WidgetRep || !eventData)
    {
    return false;
    }

  switch (this->WidgetState)
    {
    case WidgetStateRotate:
      this->ProcessRotate(eventData);
      break;
    case WidgetStateTranslate:
      {
      const double* worldPos = eventData->GetWorldPosition();
      this->GetSliceNode()->JumpAllSlices(worldPos[0], worldPos[1], worldPos[2]);
      }
      break;
    case WidgetStateMPR:
      {
      const double* worldPos = eventData->GetWorldPosition();

      if (this->DragXPrepared && this->DragYPrepared)
        {
          // Move two MPR lines
          this->GetSliceNode()->JumpAllSlices(worldPos[0], worldPos[1], worldPos[2]);
        }
      else
        {
          // Move one MPR line
          this->GetSliceNode()->JumpSlice(worldPos[0], worldPos[1], worldPos[2], this->DragXPrepared, this->DragYPrepared);
        }
      }
      break;
    case WidgetStateBlend:
      this->ProcessBlend(eventData);
      break;
    case WidgetStateTranslateSlice:
      this->ProcessTranslateSlice(eventData);
      break;
    case WidgetStateZoomSlice:
      this->ProcessZoomSlice(eventData);
      break;
    }

  return true;
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessStartMouseDrag(vtkMRMLInteractionEventData* eventData)
{
  vtkMRMLSliceIntersectionRepresentation2D* rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (!rep)
    {
    return false;
    }

  const int* displayPos = eventData->GetDisplayPosition();

  this->StartEventPosition[0] = displayPos[0];
  this->StartEventPosition[1] = displayPos[1];

  this->PreviousEventPosition[0] = this->StartEventPosition[0];
  this->PreviousEventPosition[1] = this->StartEventPosition[1];

  this->ProcessMouseMove(eventData);
  return true;
}

//----------------------------------------------------------------------------
double vtkMRMLSliceIntersectionWidget::GetSliceRotationAngleRad(double eventPos[2])
{
  return atan2(eventPos[1] - this->StartRotationCenter[1],
    eventPos[0] - this->StartRotationCenter[0]);
}


//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessEndMouseDrag(vtkMRMLInteractionEventData* vtkNotUsed(eventData))
{
  if (this->WidgetState == WidgetStateIdle)
    {
    return false;
    }
  this->SetWidgetState(WidgetStateIdle);
  return true;
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessTouchGestureStart(vtkMRMLInteractionEventData* vtkNotUsed(eventData))
{
  this->SetWidgetState(WidgetStateTouchGesture);
  return true;
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessTouchGestureEnd(vtkMRMLInteractionEventData* vtkNotUsed(eventData))
{
  this->TotalTouchRotation = 0.0;
  this->TouchRotateEnabled = false;
  this->TotalTouchZoom = 0.0;
  this->TouchZoomEnabled = false;
  this->TotalTouchTranslation = 0.0;
  this->TouchTranslationEnabled = false;
  this->SetWidgetState(WidgetStateIdle);
  return true;
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessTouchRotate(vtkMRMLInteractionEventData* eventData)
{
  this->TotalTouchRotation += eventData->GetRotation() - eventData->GetLastRotation();
  if (this->TouchRotateEnabled || std::abs(this->TotalTouchRotation) >= this->TouchRotationThreshold)
    {
    this->Rotate(vtkMath::RadiansFromDegrees(eventData->GetRotation() - eventData->GetLastRotation()));
    this->TouchRotateEnabled = true;
    }
  return true;
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessTouchZoom(vtkMRMLInteractionEventData* eventData)
{
  this->TotalTouchZoom += eventData->GetLastScale() / eventData->GetScale();
  if (this->TouchZoomEnabled || std::abs(this->TotalTouchZoom - 1.0) > this->TouchZoomThreshold)
    {
    this->ScaleZoom(eventData->GetLastScale() / eventData->GetScale(), eventData);
    this->TouchZoomEnabled = true;
    }
  return true;
}

//-------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessTouchTranslate(vtkMRMLInteractionEventData* eventData)
{
  vtkMRMLSliceNode* sliceNode = this->SliceLogic->GetSliceNode();

  vtkMatrix4x4* xyToSlice = sliceNode->GetXYToSlice();
  const double* translate = eventData->GetTranslation();
  double translation[2] = {
    xyToSlice->GetElement(0, 0) * translate[0],
    xyToSlice->GetElement(1, 1) * translate[1]
  };


  this->TotalTouchTranslation += vtkMath::Norm2D(translate);

  if (this->TouchTranslationEnabled || this->TotalTouchTranslation >= this->TouchTranslationThreshold)
    {
    double sliceOrigin[3];
    sliceNode->GetXYZOrigin(sliceOrigin);
    sliceNode->SetSliceOrigin(sliceOrigin[0] - translation[0], sliceOrigin[1] - translation[1], 0);
    this->TouchTranslationEnabled = true;
    }

  return true;
}

//----------------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::SetMRMLApplicationLogic(vtkMRMLApplicationLogic* appLogic)
{
  if (appLogic == this->ApplicationLogic)
    {
    return;
    }
  this->Superclass::SetMRMLApplicationLogic(appLogic);
  vtkCollection* sliceLogics = nullptr;
  this->SliceLogic = nullptr;
  if (appLogic)
    {
    sliceLogics = appLogic->GetSliceLogics();
    }
  if (sliceLogics != this->SliceLogics)
    {
    if (this->SliceLogics)
      {
      this->SliceLogics->RemoveObserver(this->SliceLogicsModifiedCommand);
      }
    if (sliceLogics)
      {
      sliceLogics->AddObserver(vtkCommand::ModifiedEvent, this->SliceLogicsModifiedCommand);
      }
    this->SliceLogics = sliceLogics;
    if (this->GetSliceNode())
      {
      this->SliceLogic = this->GetMRMLApplicationLogic()->GetSliceLogic(this->GetSliceNode());
      }
    }
}

//----------------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::SliceLogicsModifiedCallback(vtkObject* vtkNotUsed(caller),
  unsigned long vtkNotUsed(eid), void* clientData, void* vtkNotUsed(callData))
{
  vtkMRMLSliceIntersectionWidget* self = vtkMRMLSliceIntersectionWidget::SafeDownCast((vtkObject*)clientData);
  if (!self)
    {
    return;
    }
  vtkMRMLSliceIntersectionRepresentation2D* rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(self->WidgetRep);
  if (rep)
    {
    rep->UpdateIntersectingSliceNodes();
    }
  self->SliceLogic = nullptr;
  if (self->GetMRMLApplicationLogic())
    {
    self->SliceLogic = self->GetMRMLApplicationLogic()->GetSliceLogic(self->GetSliceNode());
    }
}

//----------------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::SetSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (this->SliceNode == sliceNode)
    {
    // no change
    return;
    }
  this->SliceNode = sliceNode;

  // Update slice logic
  this->SliceLogic = nullptr;
  if (this->GetMRMLApplicationLogic())
    {
    this->SliceLogic = this->GetMRMLApplicationLogic()->GetSliceLogic(this->SliceNode);
    }

  // Update representation
  vtkMRMLSliceIntersectionRepresentation2D* rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (rep)
    {
    rep->SetSliceNode(sliceNode);
    }
}

//----------------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLSliceIntersectionWidget::GetSliceNode()
{
  return this->SliceNode;
}

//----------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessRotateStart(vtkMRMLInteractionEventData* eventData)
{
  vtkMRMLSliceIntersectionRepresentation2D* rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (!rep)
    {
    return false;
    }

  this->SetWidgetState(WidgetStateRotate);

  // Save rotation center
  double* sliceIntersectionPoint = rep->GetSliceIntersectionPoint();
  this->StartRotationCenter[0] = sliceIntersectionPoint[0];
  this->StartRotationCenter[1] = sliceIntersectionPoint[1];
  double startRotationCenterXY[4] = { sliceIntersectionPoint[0] , sliceIntersectionPoint[1], 0.0, 1.0 };
  this->GetSliceNode()->GetXYToRAS()->MultiplyPoint(startRotationCenterXY, this->StartRotationCenter_RAS);

  // Save initial rotation angle
  const int* displayPos = eventData->GetDisplayPosition();
  double displayPosDouble[2] = { static_cast<double>(displayPos[0]), static_cast<double>(displayPos[1]) };
  this->PreviousRotationAngleRad = this->GetSliceRotationAngleRad(displayPosDouble);

  return this->ProcessStartMouseDrag(eventData);
}

//----------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessRotate(vtkMRMLInteractionEventData* eventData)
{
  vtkMRMLSliceIntersectionRepresentation2D* rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (!rep)
    {
    return false;
    }

  const int* displayPos = eventData->GetDisplayPosition();
  double displayPosDouble[2] = { static_cast<double>(displayPos[0]), static_cast<double>(displayPos[1]) };
  double sliceRotationAngleRad = this->GetSliceRotationAngleRad(displayPosDouble);

  vtkMatrix4x4* sliceToRAS = this->GetSliceNode()->GetSliceToRAS();
  vtkNew<vtkTransform> rotatedSliceToSliceTransform;

  rotatedSliceToSliceTransform->Translate(this->StartRotationCenter_RAS[0], this->StartRotationCenter_RAS[1], this->StartRotationCenter_RAS[2]);
  double rotationDirection = vtkMath::Determinant3x3(sliceToRAS->Element[0], sliceToRAS->Element[1], sliceToRAS->Element[2]) >= 0 ? 1.0 : -1.0;
  rotatedSliceToSliceTransform->RotateWXYZ(rotationDirection*vtkMath::DegreesFromRadians(sliceRotationAngleRad - this->PreviousRotationAngleRad),
    sliceToRAS->GetElement(0, 2), sliceToRAS->GetElement(1, 2), sliceToRAS->GetElement(2, 2));
  rotatedSliceToSliceTransform->Translate(-this->StartRotationCenter_RAS[0], -this->StartRotationCenter_RAS[1], -this->StartRotationCenter_RAS[2]);

  this->CurrentRotationAngleRad += ( sliceRotationAngleRad - this->PreviousRotationAngleRad );
  this->PreviousRotationAngleRad = sliceRotationAngleRad;
  this->PreviousEventPosition[0] = displayPos[0];
  this->PreviousEventPosition[1] = displayPos[1];

  rep->TransformIntersectingSlices(rotatedSliceToSliceTransform->GetMatrix());

  return true;
}

//----------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::Rotate(double sliceRotationAngleRad)
{
  vtkMRMLSliceIntersectionRepresentation2D* rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (!rep)
    {
    return false;
    }
  vtkMatrix4x4* sliceToRAS = this->GetSliceNode()->GetSliceToRAS();
  vtkNew<vtkTransform> rotatedSliceToSliceTransform;

  double* sliceIntersectionPoint = rep->GetSliceIntersectionPoint();
  double startRotationCenterXY[4] = { sliceIntersectionPoint[0] , sliceIntersectionPoint[1], 0.0, 1.0 };
  this->GetSliceNode()->GetXYToRAS()->MultiplyPoint(startRotationCenterXY, this->StartRotationCenter_RAS);

  rotatedSliceToSliceTransform->Translate(this->StartRotationCenter_RAS[0], this->StartRotationCenter_RAS[1], this->StartRotationCenter_RAS[2]);
  double rotationDirection = vtkMath::Determinant3x3(sliceToRAS->Element[0], sliceToRAS->Element[1], sliceToRAS->Element[2]) >= 0 ? 1.0 : -1.0;
  rotatedSliceToSliceTransform->RotateWXYZ(rotationDirection*vtkMath::DegreesFromRadians(sliceRotationAngleRad),
    sliceToRAS->GetElement(0, 2), sliceToRAS->GetElement(1, 2), sliceToRAS->GetElement(2, 2));
  rotatedSliceToSliceTransform->Translate(-this->StartRotationCenter_RAS[0], -this->StartRotationCenter_RAS[1], -this->StartRotationCenter_RAS[2]);

  this->PreviousRotationAngleRad = sliceRotationAngleRad;

  rep->TransformIntersectingSlices(rotatedSliceToSliceTransform->GetMatrix());
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessBlend(vtkMRMLInteractionEventData* eventData)
{
  const int* eventPosition = eventData->GetDisplayPosition();

  int* windowSize = this->Renderer->GetRenderWindow()->GetSize();
  double windowMinSize = std::min(windowSize[0], windowSize[1]);

  int deltaY = eventPosition[1] - this->PreviousEventPosition[1];
  double offsetY = (2.0 * deltaY) / windowMinSize;
  double newForegroundOpacity =
    this->LastForegroundOpacity + offsetY;
  newForegroundOpacity = std::min(std::max(newForegroundOpacity, 0.), 1.);
  vtkMRMLSliceCompositeNode *sliceCompositeNode = this->SliceLogic->GetSliceCompositeNode();
  if (sliceCompositeNode->GetForegroundVolumeID() != nullptr)
    {
    sliceCompositeNode->SetForegroundOpacity(newForegroundOpacity);
    this->LastForegroundOpacity = newForegroundOpacity;
    }
  int deltaX = eventPosition[0] - this->PreviousEventPosition[0];
  double offsetX = (2.0 * deltaX) / windowMinSize;
  double newLabelOpacity = this->LastLabelOpacity + offsetX;
  newLabelOpacity = std::min(std::max(newLabelOpacity, 0.), 1.);
  if (sliceCompositeNode->GetLabelVolumeID() != nullptr || this->StartActionSegmentationDisplayNode != nullptr)
    {
    this->SetLabelOpacity(newLabelOpacity);
    this->LastLabelOpacity = newLabelOpacity;
    }

  this->PreviousEventPosition[0] = eventPosition[0];
  this->PreviousEventPosition[1] = eventPosition[1];

  return true;
}


//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::SetLabelOpacity(double opacity)
{
  // If a labelmap node is selected then adjust opacity of that
  vtkMRMLSliceCompositeNode *sliceCompositeNode = this->SliceLogic->GetSliceCompositeNode();
  if (sliceCompositeNode->GetLabelVolumeID())
  {
    sliceCompositeNode->SetLabelOpacity(opacity);
    return;
  }
  // No labelmap node is selected, adjust segmentation node instead
  vtkMRMLSegmentationDisplayNode* displayNode = this->StartActionSegmentationDisplayNode;
  if (!displayNode)
  {
    displayNode = this->GetVisibleSegmentationDisplayNode();
  }
  if (!displayNode)
  {
    return;
  }
  displayNode->SetOpacity(opacity);
}

//----------------------------------------------------------------------------
double vtkMRMLSliceIntersectionWidget::GetLabelOpacity()
{
  // If a labelmap node is selected then get opacity of that
  vtkMRMLSliceCompositeNode *sliceCompositeNode = this->SliceLogic->GetSliceCompositeNode();
  if (sliceCompositeNode->GetLabelVolumeID())
  {
    return sliceCompositeNode->GetLabelOpacity();
  }
  // No labelmap node is selected, use segmentation node instead
  vtkMRMLSegmentationDisplayNode* displayNode = this->StartActionSegmentationDisplayNode;
  if (!displayNode)
  {
    displayNode = this->GetVisibleSegmentationDisplayNode();
  }
  if (!displayNode)
  {
    return 0;
  }
  return displayNode->GetOpacity();
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationDisplayNode* vtkMRMLSliceIntersectionWidget::GetVisibleSegmentationDisplayNode()
{
  vtkMRMLSliceNode *sliceNode = this->SliceLogic->GetSliceNode();
  vtkMRMLScene* scene = this->SliceLogic->GetMRMLScene();
  std::vector<vtkMRMLNode*> displayNodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLSegmentationDisplayNode", displayNodes) : 0;
  for (int i = 0; i < nnodes; i++)
  {
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(displayNodes[i]);
    if (displayNode
      && displayNode->GetVisibility(sliceNode->GetID())
      && (displayNode->GetVisibility2DOutline() || displayNode->GetVisibility2DFill()))
    {
      return displayNode;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
double vtkMRMLSliceIntersectionWidget::GetSliceSpacing()
{
  double spacing;
  vtkMRMLSliceNode *sliceNode = this->SliceLogic->GetSliceNode();

  if (sliceNode->GetSliceSpacingMode() == vtkMRMLSliceNode::PrescribedSliceSpacingMode)
    {
    spacing = sliceNode->GetPrescribedSliceSpacing()[2];
    }
  else
    {
    spacing = this->SliceLogic->GetLowestVolumeSliceSpacing()[2];
    }
  return spacing;
}

//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::IncrementSlice()
{
  this->MoveSlice(this->GetSliceSpacing());
}
//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::DecrementSlice()
{
  this->MoveSlice(-1. * this->GetSliceSpacing());
}
//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::MoveSlice(double delta)
{
  double offset = this->SliceLogic->GetSliceOffset();
  double newOffset = offset + delta;

  double sliceBounds[6] = {0, -1, 0, -1, 0, -1};
  this->SliceLogic->GetSliceBounds(sliceBounds);
  if (newOffset >= sliceBounds[4] && newOffset <= sliceBounds[5])
    {
    this->SliceLogic->StartSliceNodeInteraction(vtkMRMLSliceNode::SliceToRASFlag);
    this->SliceLogic->SetSliceOffset(newOffset);
    this->SliceLogic->EndSliceNodeInteraction();

    // this->UpdateCrosshairOffset(delta);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::CycleVolumeLayer(int layer, int direction)
{
  // first, find the current volume index for the given layer (can be nullptr)
  vtkMRMLScene *scene = this->SliceLogic->GetMRMLScene();
  vtkMRMLSliceCompositeNode *sliceCompositeNode = this->SliceLogic->GetSliceCompositeNode();
  char *volumeID = nullptr;
  switch (layer)
    {
    case 0: { volumeID = sliceCompositeNode->GetBackgroundVolumeID(); } break;
    case 1: { volumeID = sliceCompositeNode->GetForegroundVolumeID(); } break;
    case 2: { volumeID = sliceCompositeNode->GetLabelVolumeID(); } break;
    }
  vtkMRMLVolumeNode *volumeNode = vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(volumeID));

  // now figure out which one it is in the list
  int volumeCount = scene->GetNumberOfNodesByClass("vtkMRMLVolumeNode");
  int volumeIndex;
  for (volumeIndex = 0; volumeIndex < volumeCount; volumeIndex++)
    {
    if (volumeNode == scene->GetNthNodeByClass(volumeIndex, "vtkMRMLVolumeNode"))
      {
      break;
      }
    }

  // now increment by direction, and clamp to number of nodes
  volumeIndex += direction;
  if (volumeIndex >= volumeCount)
    {
    volumeIndex = 0;
    }
  if (volumeIndex < 0)
    {
    volumeIndex = volumeCount - 1;
    }

  // if we found a node, set it in the given layer
  volumeNode = vtkMRMLVolumeNode::SafeDownCast(
                  scene->GetNthNodeByClass(volumeIndex, "vtkMRMLVolumeNode"));
  if (volumeNode)
    {
    switch (layer)
      {
      case 0: { sliceCompositeNode->SetBackgroundVolumeID(volumeNode->GetID()); } break;
      case 1: { sliceCompositeNode->SetForegroundVolumeID(volumeNode->GetID()); } break;
      case 2: { sliceCompositeNode->SetLabelVolumeID(volumeNode->GetID()); } break;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessTranslateSlice(vtkMRMLInteractionEventData* eventData)
{
  vtkMRMLSliceNode *sliceNode = this->SliceLogic->GetSliceNode();

  double xyz[3];
  sliceNode->GetXYZOrigin(xyz);

  const int* eventPosition = eventData->GetDisplayPosition();

  // account for zoom using XYToSlice matrix
  vtkMatrix4x4* xyToSlice = sliceNode->GetXYToSlice();
  double deltaX = xyToSlice->GetElement(0, 0)*(this->PreviousEventPosition[0] - eventPosition[0]);
  double deltaY = xyToSlice->GetElement(1, 1)*(this->PreviousEventPosition[1] - eventPosition[1]);

  sliceNode->SetSliceOrigin(xyz[0] + deltaX, xyz[1] + deltaY, 0);

  this->PreviousEventPosition[0] = eventPosition[0];
  this->PreviousEventPosition[1] = eventPosition[1];
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessZoomSlice(vtkMRMLInteractionEventData* eventData)
{
  const int* eventPosition = eventData->GetDisplayPosition();

  int* windowSize = this->GetRenderer()->GetRenderWindow()->GetSize();

  int deltaY = eventPosition[1] - this->StartEventPosition[1];
  double percent = (windowSize[1] + deltaY) / (1.0 * windowSize[1]);

  // the factor operation is so 'z' isn't changed and the
  // slider can still move through the full range
  if (percent > 0.)
    {
    double newFOVx = this->StartActionFOV[0] * percent;
    double newFOVy = this->StartActionFOV[1] * percent;
    double newFOVz = this->StartActionFOV[2];
    vtkMRMLSliceNode *sliceNode = this->SliceLogic->GetSliceNode();
    sliceNode->SetFieldOfView(newFOVx, newFOVy, newFOVz);
    sliceNode->UpdateMatrices();
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::ScaleZoom(double zoomScaleFactor, vtkMRMLInteractionEventData* eventData)
{
  // the factor operation is so 'z' isn't changed and the
  // slider can still move through the full range
  if (zoomScaleFactor <= 0)
    {
    vtkWarningMacro("vtkMRMLSliceViewInteractorStyle::ScaleZoom: invalid zoom scale factor (" << zoomScaleFactor);
    return;
    }
  this->SliceLogic->StartSliceNodeInteraction(vtkMRMLSliceNode::FieldOfViewFlag);
  vtkMRMLSliceNode *sliceNode = this->SliceLogic->GetSliceNode();
  int wasModifying = sliceNode->StartModify();

  // Get distance of event position from slice center
  const int* eventPosition = eventData->GetDisplayPosition();
  int* windowSize = this->GetRenderer()->GetRenderWindow()->GetSize();
  vtkMatrix4x4* xyToSlice = sliceNode->GetXYToSlice();
  double evenPositionDistanceFromOrigin[2] =
    {
    (eventPosition[0] - windowSize[0] / 2) * xyToSlice->GetElement(0, 0),
    (eventPosition[1] - windowSize[1] / 2) * xyToSlice->GetElement(1, 1)
    };

  // Adjust field of view
  double fov[3] = { 1.0 };
  sliceNode->GetFieldOfView(fov);
  fov[0] *= zoomScaleFactor;
  fov[1] *= zoomScaleFactor;
  sliceNode->SetFieldOfView(fov[0], fov[1], fov[2]);

  // Keep the mouse position at the same place on screen
  double sliceOrigin[3] = { 0 };
  sliceNode->GetXYZOrigin(sliceOrigin);
  sliceNode->SetSliceOrigin(
    sliceOrigin[0] + evenPositionDistanceFromOrigin[0] * (1.0 - zoomScaleFactor),
    sliceOrigin[1] + evenPositionDistanceFromOrigin[1] * (1.0 - zoomScaleFactor),
    sliceOrigin[2]);

  sliceNode->UpdateMatrices();
  sliceNode->EndModify(wasModifying);
  this->SliceLogic->EndSliceNodeInteraction();
}

//----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::ProcessSetCrosshair(vtkMRMLInteractionEventData* eventData)
{
  if (!this->ModifierKeyPressedSinceLastMouseButtonRelease)
    {
    // this event was caused by a "stuck" modifier key
    return false;
    }
  vtkMRMLSliceNode* sliceNode = this->GetSliceNode();
  vtkMRMLCrosshairNode* crosshairNode = vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(sliceNode->GetScene());
  if (!crosshairNode)
    {
    return false;
    }
  const double* worldPos = eventData->GetWorldPosition();
  crosshairNode->SetCrosshairRAS(const_cast<double*>(worldPos));
  if (crosshairNode->GetCrosshairBehavior() != vtkMRMLCrosshairNode::NoAction)
    {
    int viewJumpSliceMode = vtkMRMLSliceNode::OffsetJumpSlice;
    if (crosshairNode->GetCrosshairBehavior() == vtkMRMLCrosshairNode::CenteredJumpSlice)
      {
      viewJumpSliceMode = vtkMRMLSliceNode::CenteredJumpSlice;
      }
    sliceNode->JumpAllSlices(sliceNode->GetScene(),
      worldPos[0], worldPos[1], worldPos[2],
      viewJumpSliceMode, sliceNode->GetViewGroup(), sliceNode);
    }
  return true;
}

void vtkMRMLSliceIntersectionWidget::UpdateCrosshair(vtkMRMLInteractionEventData *eventData)
{
  return;
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
  vtkMRMLCrosshairNode *crosshairNode = vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(sliceNode->GetScene());
  if (!crosshairNode)
  {
    return;
  }
  double *worldPos = (double *)eventData->GetWorldPosition();

  crosshairNode->SetCrosshairRAS(const_cast<double*>(worldPos));
}

void vtkMRMLSliceIntersectionWidget::UpdateCrosshairMPR(vtkMRMLInteractionEventData *eventData)
{
  return;
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
  vtkMRMLCrosshairNode *crosshairNode = vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(sliceNode->GetScene());
  if (!crosshairNode)
  {
    return;
  }
  double *worldPos = (double *)eventData->GetWorldPosition();
  double ras[3];
  //crosshairNode->GetCrossHairRAS(ras);

  int n = sliceNode->GetNodeIndex();
  switch (n)
  {
  case 0:
    if (this->DragXPrepared)
      ras[1] = worldPos[1];
    if (this->DragYPrepared)
      ras[0] = worldPos[0];
    break;
  case 1:
    if (this->DragXPrepared)
      ras[2] = worldPos[2];
    if (this->DragYPrepared)
      ras[1] = worldPos[1];
    break;
  case 2:
    if (this->DragXPrepared)
      ras[2] = worldPos[2];
    if (this->DragYPrepared)
      ras[0] = worldPos[0];
    break;
  }

  crosshairNode->SetCrosshairRAS(ras);
}

void vtkMRMLSliceIntersectionWidget::UpdateCrosshairOffset(double delta)
{
  return;
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();
  vtkMRMLCrosshairNode *crosshairNode = vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(sliceNode->GetScene());
  if (!crosshairNode)
  {
    return;
  }

  double ras[3];
  //crosshairNode->GetCrossHairRAS(ras);

  int n = sliceNode->GetNodeIndex();
  switch (n)
  {
    case 0:
      ras[2] += delta;
      break;
    case 1:
      ras[0] += delta;
      break;
    case 2:
      ras[1] += delta;
      break;
  }

  crosshairNode->SetCrosshairRAS(ras);
}

// Check if mouse cursor is on the MPR line and setup flag and invoke event
bool vtkMRMLSliceIntersectionWidget::IsCursorOutSlice(double *cursorRas)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  vtkMatrix4x4 *xyzToRAS = sliceNode->GetXYToRAS();
  int *dims = sliceNode->GetDimensions();
  double p1xyz[4] = {0.0, 0.0, 0.0, 1.0};
  double p2xyz[4] = {dims[0], dims[1], 0.0, 1.0};
  double bottomLeft[4], topRight[4];

  xyzToRAS->MultiplyPoint(p1xyz, bottomLeft);

  double dx1, dy1, dx2, dy2;
  int n = sliceNode->GetNodeIndex();
  switch (n)
  {
    case 0: // Axial
      xyzToRAS->MultiplyPoint(p2xyz, topRight);
      dx1 = cursorRas[0] - bottomLeft[0];
      dy1 = cursorRas[1] - bottomLeft[1];
      dx2 = cursorRas[0] - topRight[0];
      dy2 = cursorRas[1] - topRight[1];
      break;
    case 1: // Sagittal
      xyzToRAS->MultiplyPoint(p2xyz, topRight);
      dx1 = cursorRas[1] - bottomLeft[1];
      dy1 = cursorRas[2] - bottomLeft[2];
      dx2 = cursorRas[1] - topRight[1];
      dy2 = cursorRas[2] - topRight[2];
      break;
    case 2: // Coronal
      xyzToRAS->MultiplyPoint(p2xyz, topRight);
      dx1 = cursorRas[0] - bottomLeft[0];
      dy1 = cursorRas[2] - bottomLeft[2];
      dx2 = cursorRas[0] - topRight[0];
      dy2 = cursorRas[2] - topRight[2];
      break;
    default:
      return false;
  }

  // outputLogDouble("------- bounds --", bottomLeft);
  // outputLogDouble("------- bounds --", topRight);
  if (std::abs(dx1) < this->TouchMoveThreshold ||
      std::abs(dy1) < this->TouchMoveThreshold ||
      std::abs(dx2) < this->TouchMoveThreshold ||
      std::abs(dy2) < this->TouchMoveThreshold)
  {
    return true;
  }
  return false;
}
// Check if mouse cursor is out of slice
bool vtkMRMLSliceIntersectionWidget::IsCursorOutSliceXY(int *cursorPos)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  int *dims = sliceNode->GetDimensions();

  if (cursorPos[0] < this->TouchMoveThreshold ||
      cursorPos[1] < this->TouchMoveThreshold ||
      cursorPos[0] > dims[0] - this->TouchMoveThreshold ||
      cursorPos[1] > dims[1] - this->TouchMoveThreshold)
  {
    return true;
  }

  return false;
}
bool vtkMRMLSliceIntersectionWidget::GetDistanceFromAxis(double *cursorPos, double *x, double *y)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  // char orient[32];
  // strcpy(orient, sliceNode->GetOrientation().c_str());
  double cross[3];
  double deltaX = 1000, deltaY = 1000;

  vtkMRMLSliceIntersectionRepresentation2D *rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (!rep)
  {
    return false;
  }
  double *sliceIntersectionPoint = rep->GetSliceIntersectionPoint();

  double ras[3], xyz[3];
  xyz[0] = sliceIntersectionPoint[0];
  xyz[1] = sliceIntersectionPoint[1];
  xyz[2] = sliceIntersectionPoint[2];
  vtkMRMLCrosshairDisplayableManager::ConvertXYZToRAS(sliceNode, xyz, cross);
  // outputLogDouble("sliceIntersectionPoint", cross);

  int n = sliceNode->GetNodeIndex();
  // Get distance from the MPR line to the cursor
  switch (n)
  {
    case 0: // Axial
      deltaX = -(cursorPos[0] - cross[0]);
      deltaY = cursorPos[1] - cross[1];
      break;
    case 1: // Sagittal
      deltaX = -(cursorPos[1] - cross[1]);
      deltaY = cursorPos[2] - cross[2];
      break;
    case 2: // Coronal
      deltaX = -(cursorPos[0] - cross[0]);
      deltaY = cursorPos[2] - cross[2];
      break;
    default:
      return false;
  }
  // outputLogDoubleTwo("---- delta", deltaX, deltaY);

  if (deltaX != 0.0)
  {
    double dxy = sqrt(deltaX * deltaX + deltaY * deltaY);
    double dtan = atan2(deltaY, deltaX);
    deltaX = dxy * cos(dtan - this->CurrentRotationAngleRad);
    deltaY = dxy * sin(dtan - this->CurrentRotationAngleRad);

  // outputLogDoubleTwo("-------- Angle delta", deltaX, deltaY);
  }
  *x = deltaX;
  *y = deltaY;

  return true;
}

// Get distance between mouse cursor and axes in XY
bool vtkMRMLSliceIntersectionWidget::GetDistanceFromAxisXY(int *cursorPos, double *x, double *y)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  double cross[3];
  double deltaX = 1000, deltaY = 1000;

  vtkMRMLSliceIntersectionRepresentation2D *rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
  if (!rep)
  {
    return false;
  }
  double *sliceIntersectionPoint = rep->GetSliceIntersectionPoint();

  double xyz[3];
  xyz[0] = sliceIntersectionPoint[0];
  xyz[1] = sliceIntersectionPoint[1];
  xyz[2] = sliceIntersectionPoint[2];
  // vtkMRMLCrosshairDisplayableManager::ConvertXYZToRAS(sliceNode, xyz, cross);
  // outputLogDouble("------- crosshair in xyz", xyz);

  deltaX = cursorPos[0] - xyz[0];
  deltaY = cursorPos[1] - xyz[1];

  // outputLogDoubleTwo("---- delta", deltaX, deltaY);

  if (deltaX != 0.0)
  {
    double dxy = sqrt(deltaX * deltaX + deltaY * deltaY);
    double dtan = atan2(deltaY, deltaX);
    deltaX = dxy * cos(dtan - this->CurrentRotationAngleRad);
    deltaY = dxy * sin(dtan - this->CurrentRotationAngleRad);

   // outputLogDoubleTwo("-------- Angle delta", deltaX, deltaY);
  }
  *x = deltaX;
  *y = deltaY;

  return true;
}

// Check if mouse cursor is on the MPR line and setup flag and invoke event
bool vtkMRMLSliceIntersectionWidget::ProcessMPR(vtkMRMLInteractionEventData *eventData)
{
  vtkMRMLSliceNode *sliceNode = this->GetSliceNode();

  // double *cursorPos = (double *)eventData->GetWorldPosition();
  // outputLogDouble("-- cursor --", cursorPos);
  int cursorDisp[2];
  eventData->GetDisplayPosition(cursorDisp);
  // outputLogIntDouble("-- cursor disp --", cursorDisp);

  bool oldDragXPrepared = this->DragXPrepared;
  bool oldDragYPrepared = this->DragYPrepared;

  double deltaX, deltaY;

  this->DragXPrepared = false;
  this->DragYPrepared = false;
  double dtan;

  if (this->IsCursorOutSliceXY(cursorDisp))
  {
  }
  // else if (GetDistanceFromAxis(cursorPos, & deltaX, &deltaY))
  // else if (GetDistanceFromAxisXY(cursorDisp, &deltaX, &deltaY))
  else
  {
    vtkMRMLSliceIntersectionRepresentation2D *rep = vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
    if (!rep)
    {
      return true;
    }
    double *sliceIntersectionPoint = rep->GetSliceIntersectionPoint();

    double xyz[3];
    xyz[0] = sliceIntersectionPoint[0];
    xyz[1] = sliceIntersectionPoint[1];
    xyz[2] = sliceIntersectionPoint[2];

    if (std::abs(xyz[0] - cursorDisp[0]) < this->TouchMoveThreshold && std::abs(xyz[1] - cursorDisp[1]) < this->TouchMoveThreshold)
    {
      this->DragXPrepared = true;
      this->DragYPrepared = true;
    }
    else
    {
      int opt = rep->CheckOnMPRLines(cursorDisp, this->TouchMoveThreshold, &dtan);
      if (opt >= 0)
      {
        int sliceIndex = sliceNode->GetNodeIndex();
        if (sliceIndex == 0)
        {   // Axial
          this->DragXPrepared = (opt == 1);
          this->DragYPrepared = (opt == 0);
        }
        else
        {   // Sagittal, Coronal
          this->DragXPrepared = (opt == 0);
          this->DragYPrepared = (opt == 1);        
        }        
      }
    }
    // this->DragXPrepared = (std::abs(deltaY) < this->TouchMoveThreshold);
    // this->DragYPrepared = (std::abs(deltaX) < this->TouchMoveThreshold);
  }

  // if there's no change with crossing status then return without changing
  // if (oldDragXPrepared == this->DragXPrepared && oldDragYPrepared == this->DragYPrepared)
  //   return false;

  vtkMRMLCrosshairNode *crosshairNode = vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(sliceNode->GetScene());
  if (crosshairNode == nullptr) return true;

  if (this->DragXPrepared && this->DragYPrepared)
  {
    crosshairNode->InvokeEvent(vtkMRMLCrosshairNode::CursorShapeCenterEvent, nullptr);
  }
  else if (!this->DragXPrepared && !this->DragYPrepared)
  {
    crosshairNode->InvokeEvent(vtkMRMLCrosshairNode::CursorShapeRestoreEvent, nullptr);
  }
  else
  {
    double nn = dtan / vtkMath::RadiansFromDegrees(30.0);
    while (nn < 0)
    {
      nn += 12;
    }        // make positive angle to mod operation
    int n = (int) nn;
    n %= 12; // make angle from 0 to 360 and divide 30

    int event;
    if (n == 0 || n == 5 || n == 6 || n == 11)
      event = vtkMRMLCrosshairNode::CursorShapeVerEvent;
    else if (n == 2 || n == 3 || n == 8 || n == 9)
      event = vtkMRMLCrosshairNode::CursorShapeHorEvent;
    else if (n == 1 || n == 7)
      event = vtkMRMLCrosshairNode::CursorShapeNWEvent;
    else if (n == 4 || n == 10)
      event = vtkMRMLCrosshairNode::CursorShapeSWEvent;
    if (this->DragXPrepared)
    {
      crosshairNode->InvokeEvent(event, nullptr);
    }
    else
    {
      crosshairNode->InvokeEvent(event, nullptr);
    }
  }

  return true;
}

vtkMRMLSliceIntersectionRepresentation2D *vtkMRMLSliceIntersectionWidget::GetIntersectionRepresentation2D()
{
  // return vtkSlicerMarkupsWidgetRepresentation::SafeDownCast(this->WidgetRep);
  return vtkMRMLSliceIntersectionRepresentation2D::SafeDownCast(this->WidgetRep);
}

// Currently this code is not working
int vtkMRMLSliceIntersectionWidget::GetMouseCursor()
{
  if (this->DragXPrepared && this->DragYPrepared)
  {
    // not working currently
    return VTK_CURSOR_CROSSHAIR;
  }
  else if (this->DragXPrepared)
  {
    return VTK_CURSOR_SIZENS;
  }
  else if (this->DragYPrepared)
  {
    return VTK_CURSOR_SIZEWE;
  }
  
  return VTK_CURSOR_DEFAULT;
}

//----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::IsEventInsideVolume(bool background, vtkMRMLInteractionEventData* eventData)
{
  vtkMRMLSliceNode *sliceNode = this->SliceLogic->GetSliceNode();
  if (!sliceNode)
  {
    return false;
  }
  vtkMRMLSliceLayerLogic* layerLogic = background ?
    this->SliceLogic->GetBackgroundLayer() : this->SliceLogic->GetForegroundLayer();
  if (!layerLogic)
  {
    return false;
  }
  vtkMRMLVolumeNode* volumeNode = layerLogic->GetVolumeNode();
  if (!volumeNode || !volumeNode->GetImageData())
  {
    return false;
  }
  const int* eventPosition = eventData->GetDisplayPosition();
  double xyz[3] = { 0 };
  vtkMRMLAbstractSliceViewDisplayableManager::ConvertDeviceToXYZ(this->GetRenderer(), sliceNode, eventPosition[0], eventPosition[1], xyz);
  vtkGeneralTransform* xyToBackgroundIJK = layerLogic->GetXYToIJKTransform();
  double mousePositionIJK[3] = { 0 };
  xyToBackgroundIJK->TransformPoint(xyz, mousePositionIJK);
  int volumeExtent[6] = { 0 };
  volumeNode->GetImageData()->GetExtent(volumeExtent);
  for (int i = 0; i < 3; i++)
  {
    if (mousePositionIJK[i]<volumeExtent[i * 2] || mousePositionIJK[i]>volumeExtent[i * 2 + 1])
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLSliceIntersectionWidget::GetActionEnabled(int actionsMask)
{
  return (this->ActionsEnabled & actionsMask) == actionsMask;
}

//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::SetActionEnabled(int actionsMask, bool enable /*=true*/)
{
  if (enable)
    {
    this->ActionsEnabled |= actionsMask;
    }
  else
    {
    this->ActionsEnabled &= (~actionsMask);
    }
  this->UpdateInteractionEventMapping();
}

//----------------------------------------------------------------------------
int vtkMRMLSliceIntersectionWidget::GetActionsEnabled()
{
  return this->ActionsEnabled;
}

//----------------------------------------------------------------------------
void vtkMRMLSliceIntersectionWidget::SetActionsEnabled(int actions)
{
  this->ActionsEnabled = actions;
  this->UpdateInteractionEventMapping();
}
