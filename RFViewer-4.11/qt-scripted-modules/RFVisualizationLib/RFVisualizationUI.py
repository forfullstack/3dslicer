import ctk
import qt
import slicer
from enum import unique, Enum
from RFReconstruction import RFReconstructionLogic


from RFViewerHomeLib import translatable, toggleCheckBox, createButton
from RFVisualizationLib import RFLayoutType

@unique
class IndustryType(Enum):
  Medical = 0
  Dental = 1


@translatable
class RFVisualizationUI(qt.QWidget):

  def __init__(self, vrLogic, preset, industryType):
    qt.QWidget.__init__(self)

    self._layout = qt.QFormLayout()
    self._layout.setMargin(10)
    self._layout.setVerticalSpacing(15)
    self.spacing = 7
    self.industry = industryType

    self.addLayoutSection()
    self.addCropSection()
    self.add3DSection(vrLogic, preset)
    self.addIntermediateSection()
    self.add2DSection()
    self.addMagnificationSection()
    self.addAdvancedSection()

    self.setLayout(self._layout)

  def setVolumeNode(self, volumeNode, displayNode3D, isLoadingState):
    # Select volume in VolumeRenderingWidget selector for volume cropping and deactivate previous cropping settings
    self._volumeSelector.setCurrentNode(volumeNode)

    # Create ROI for the current display node if it doesn't exist (avoids crop volume logic crash)
    if not displayNode3D.GetROINodeID():
      roi_node = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLAnnotationROINode")
      roi_node.SetInteractiveMode(1)
      displayNode3D.SetAndObserveROINodeID(roi_node.GetID())

    # Deactivate checkboxes (and toggle them) to make sure the parameter is correctly applied
    # When loading a session, if the checkbox status are not toggled, the correct check status may be selected but not
    # propagated to the volume
    for checkBox in [self._cropCheckBox, self._displayROICheckBox, self.displayResliceCursorCheckbox,
                     self.synchronizeCheckbox]:
      toggleCheckBox(checkBox, lastCheckedState=False)

    # Fit Volume ROI to volume (by default ROI is 0)
    if not isLoadingState:
      self._fitToVolumeButton.click()

    # Set window level and scalar mapping widgets input nodes
    self.windowLevelWidget.setMRMLVolumeNode(volumeNode)
    self.scalarMappingWidget.setMRMLVolumePropertyNode(displayNode3D.GetVolumePropertyNode() if displayNode3D else None)

  def addLayoutSection(self):
    self.layoutSection = qt.QFormLayout()
    self.setLayoutSpacing(self.layoutSection)

    self.layoutSelector = self.createLayoutComboBox()
    self.layoutSection.addRow(self.tr("Layout: "), self.layoutSelector)
    self._layout.addRow(self.layoutSection)

  def addCropSection(self):
    self._volumeRenderingWidget = slicer.util.getNewModuleGui(slicer.modules.volumerendering)
    self._volumeSelector = slicer.util.findChild(self._volumeRenderingWidget, "VolumeNodeComboBox")

    self._cropCheckBox = slicer.util.findChild(self._volumeRenderingWidget, "ROICropCheckBox")
    self._cropCheckBox.text = self.tr("Enabled")

    self._displayROICheckBox = slicer.util.findChild(self._volumeRenderingWidget, "ROICropDisplayCheckBox")
    self._displayROICheckBox.text = self.tr("Display ROI")

    self._fitToVolumeButton = slicer.util.findChild(self._volumeRenderingWidget, "ROIFitPushButton")
    self._fitToVolumeButton.text = self.tr("Fit to Volume")

    layout = qt.QHBoxLayout()
    # layout.addWidget(qt.QLabel(self.tr("Crop:")))
    # layout.addWidget(self._cropCheckBox)
    layout.addWidget(self._displayROICheckBox)
    layout.addWidget(self._fitToVolumeButton)

    self._layout.addRow(layout)

  def add2DSection(self):
    self.layout2D = qt.QFormLayout()
    self.setLayoutSpacing(self.layout2D)

    self.preset2DSelector = self.create2DPresetComboBox()
    # self.layout2D.addRow(self.tr("Presets 2D: "), self.preset2DSelector)

    self.windowLevelWidget = self.createWindowLevelWidget()
    self.layout2D.addRow(self.tr("2D Window/Level: "), self.windowLevelWidget)

    self.slabThicknessSlider = self.createSlabThicknessSlider()

    layoutMIP = qt.QFormLayout()
    layoutMIP.addRow( self.slabThicknessSlider)

    layout3Buttons = qt.QHBoxLayout()
    layout3Buttons.setSpacing(20)
    layout3Buttons.addWidget(createButton(self.tr("2"), self.setMIPThickness2Button))
    layout3Buttons.addWidget(createButton(self.tr("5"), self.setMIPThickness5Button))
    layout3Buttons.addWidget(createButton(self.tr("10"), self.setMIPThickness10Button))

    self.thicknessText = qt.QLabel()
    layout3Buttons.addWidget(self.thicknessText)
    self.setMIPThickness5Button()   # default thickness as 5

    self.thicknessSelector = self.createMIPThicknessCombobox()
    layout3Buttons.addWidget(self.thicknessSelector)
    layoutMIP.addRow(layout3Buttons)
    self.layout2D.addRow(self.tr("MIP thickness: "), layoutMIP)
    self._layout.addRow(self.layout2D)

  def addMagnificationSection(self):
    self.layoutMagnification = qt.QFormLayout()
    self.setLayoutSpacing(self.layoutMagnification)

    layoutButtons = qt.QHBoxLayout()
    layoutButtons.setSpacing(20);
    layoutButtons.addWidget(createButton(self.tr("Cephalometric"), self.setCephalometricButton))
    layoutButtons.addWidget(createButton(self.tr("Size of 1:1"), self.setSizeOfButton))

    self.layoutMagnification.addRow(self.tr("Magnification: "), layoutButtons)

    self._layout.addRow(self.layoutMagnification)

  def setMIPThickness2Button(self):
    thickness = 2
    self.thicknessText.setText(str(thickness) + " mm")
    sliceNodes = slicer.util.getNodesByClass('vtkMRMLSliceNode')
    for slice in sliceNodes:
      slice.SetMipThickness(thickness)

  def setMIPThickness5Button(self):
    thickness = 5
    self.thicknessText.setText(str(thickness) + " mm")
    sliceNodes = slicer.util.getNodesByClass('vtkMRMLSliceNode')
    for slice in sliceNodes:
      slice.SetMipThickness(thickness)

  def setMIPThickness10Button(self):
    thickness = 10
    self.thicknessText.setText(str(thickness) + " mm")
    sliceNodes = slicer.util.getNodesByClass('vtkMRMLSliceNode')
    for slice in sliceNodes:
      slice.SetMipThickness(thickness)

  def setCephalometricButton(self):
    temp = 0

  def setSizeOfButton(self):
    temp = 1

  def add3DSection(self, vrLogic, preset):
    self.layout3D = qt.QFormLayout()
    self.setLayoutSpacing(self.layout3D)

    self.preset3DSelector = self.createPresetComboBox(vrLogic, preset)
    self.layout3D.addRow(self.tr("Presets 3D: "), self.preset3DSelector)

    self.shiftSlider = self.createShiftPresetSlider()
    self.layout3D.addRow(self.tr("Threshold VR: "), self.shiftSlider)

    self.colorSelector = self.createLookupTableComboBox()
    # self.layout3D.addRow(self.tr("Color Presets: "), self.colorSelector)

    self.vrModeSelector = self.createVolumeRenderingModeComboBox()
    # self.layout3D.addRow(self.tr("VR Mode: "), self.vrModeSelector)

    self._layout.addRow(self.layout3D)

  def addIntermediateSection(self):
    self.layoutIntermediate = qt.QFormLayout()
    self.setLayoutSpacing(self.layoutIntermediate)

    self.synchronizeCheckbox = qt.QCheckBox()
    # self.layoutIntermediate.addRow(self.tr("Synchronize 2D/3D colors: "), self.synchronizeCheckbox)

    self.displayResliceCursorCheckbox = qt.QCheckBox()
    self.layoutIntermediate.addRow(self.tr("Show MPR: "), self.displayResliceCursorCheckbox)

    self._layout.addRow(self.layoutIntermediate)

  def addAdvancedSection(self):
    self.layoutAdvanced = qt.QFormLayout()
    self.setLayoutSpacing(self.layoutAdvanced)

    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = self.tr("Advanced")
    parametersCollapsibleButton.collapsed = True
    self.layoutAdvanced.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayoutAdvanced = qt.QFormLayout(parametersCollapsibleButton)
    parametersCollapsibleButton.hide()
    #
    # Color/opacity scalar mapping
    #
    self.scalarMappingWidget = slicer.qMRMLVolumePropertyNodeWidget()
    parametersFormLayoutAdvanced.addWidget(self.scalarMappingWidget)

    self._layout.addRow(self.layoutAdvanced)

  def setLayoutSpacing(self, layout):
    layout.setHorizontalSpacing(self.spacing)

  def _addSeparator(self):
    separator = qt.QFrame()
    separator.setFrameShape(qt.QFrame.HLine)
    separator.setFrameShadow(qt.QFrame.Sunken)
    self._layout.addWidget(separator)

  def createLayoutComboBox(self):
    """
    Create a combobox to choose the display layout
    """
    selector = qt.QComboBox()
    selector.setToolTip(self.tr("Select the display .layout."))
    selector.addItem(self.tr("Four-up"), RFLayoutType.RFDefaultLayout)
    selector.addItem(self.tr("Main 3D"), RFLayoutType.RFMain3DLayout)
    selector.addItem(self.tr("Main Axial"), RFLayoutType.RFMainAxialLayout)
    selector.addItem(self.tr("Main Sagittal"), RFLayoutType.RFMainSagittalLayout)
    selector.addItem(self.tr("Main Coronal"), RFLayoutType.RFMainCoronalLayout)
    selector.addItem(self.tr("Conventional"), RFLayoutType.RFConventional)
    # selector.addItem(self.tr("Dual 3D"), RFLayoutType.RFDual3D)
    # selector.addItem(self.tr("Triple 3D"), RFLayoutType.RFTriple3D)
    selector.addItem(self.tr("3D Only"), RFLayoutType.RF3DOnly)
    selector.addItem(self.tr("Line Profile"), RFLayoutType.RFLineProfileLayout)
    selector.addItem(self.tr("Panorama"), RFLayoutType.RFPanoramaLayout)
    selector.addItem(self.tr("2 x 2"), RFLayoutType.RF2X2Layout)
    selector.addItem(self.tr("3 x 3"), RFLayoutType.RF3X3Layout)
    selector.addItem(self.tr("4 x 4"), RFLayoutType.RF4X4Layout)
    selector.addItem(self.tr("5 x 5"), RFLayoutType.RF5X5Layout)
    # selector.addItem(self.tr("6 x 6"), RFLayoutType.RF6X6Layout)
    # selector.addItem(self.tr("7 x 7"), RFLayoutType.RF7X7Layout)
    # selector.addItem(self.tr("8 x 8"), RFLayoutType.RF8X8Layout)
    selector.setCurrentText(self.tr("Main 3D"))
    return selector

  def createPresetComboBox(self, vrLogic, currentPreset):
    """
    Create a slicer combobox to choose the preset
    that will be applied on the volume rendering
    """

    # define removable presets
    presetsToRemove = ["MR-Angio", "MR-Default", "MR-MIP", "MR-T2-Brain", "DTI-FA-Brain"]

    # if self.industry == IndustryType.Medical:
    #   presetsToRemove += ["CT-Soft-Tissue", "CT-Air"]
    # if self.industry == IndustryType.Dental:
    #   presetsToRemove += ["CT-AAA", "CT-AAA2", "CT-Bone", "CT-Bones", "CT-Cardiac", "CT-Cardiac2",
    #   "CT-Cardiac3", "CT-Chest-Contrast-Enhanced", "CT-Chest-Vessels", "CT-Coronary-Arteries",
    #   "CT-Coronary-Arteries-2", "CT-Coronary-Arteries-3", "CT-Cropped-Volume-Bone", "CT-Fat",
    #   "CT-Liver-Vasculature", "CT-Lung", "CT-MIP", "CT-Muscle", "CT-Pulmonary-Arteries"]

    for preset in presetsToRemove:
      presetNode = vrLogic.GetPresetByName(preset)
      vrLogic.RemovePreset(presetNode)

    selector = slicer.qSlicerPresetComboBox()
    selector.nodeTypes = ["vtkMRMLVolumePropertyNode"]
    selector.setMRMLScene(vrLogic.GetPresetsScene())
    selector.showIcons = False
    selector.setCurrentNode(currentPreset)
    return selector

  def create2DPresetComboBox(self):
    selector = qt.QComboBox()
    selector.setToolTip(self.tr("Select the 2D preset."))
    if self.industry == IndustryType.Medical:
      selector.addItem("CT-Abdomen")
      selector.addItem("CT-Lung")
      selector.addItem("CT-Brain")
    elif self.industry == IndustryType.Dental:
      selector.addItem("CT-Bone")
      selector.addItem("CT-Air")
    selector.setCurrentIndex(0)
    return selector

  def createVolumeRenderingModeComboBox(self):
    """
    Create a combobox that will allow to choose the volume rendering
    visualization mode : Default or X-Ray
    """
    selector = qt.QComboBox()
    selector.setToolTip(self.tr("Select the visualization rendering mode"))
    selector.addItem(self.tr("Default VR"), slicer.vtkMRMLViewNode().Composite)
    selector.addItem(self.tr("X-Ray VR"), slicer.vtkMRMLViewNode().MaximumIntensityProjection)
    selector.setCurrentIndex(0)
    return selector

  def createLookupTableComboBox(self):
    """
    Create a combobox which lists the available lookup table
    that can be applied to the rendering
    """
    selector = qt.QComboBox()
    selector.setToolTip(self.tr("Select the color preset"))
    selector.addItem(self.tr("Current 3D preset color"), None)
    selector.addItem(self.tr("Grey"), slicer.vtkMRMLColorTableNode().Grey)
    selector.addItem(self.tr("Rainbow"), slicer.vtkMRMLColorTableNode().Rainbow)
    selector.addItem(self.tr("Inverse Rainbow"), slicer.vtkMRMLColorTableNode().ReverseRainbow)
    selector.addItem(self.tr("Bright Red"), slicer.vtkMRMLColorTableNode().Red)
    selector.addItem(self.tr("Bright Blue"), slicer.vtkMRMLColorTableNode().Blue)
    selector.addItem(self.tr("Bright Green"), slicer.vtkMRMLColorTableNode().Green)
    selector.setCurrentIndex(0)

    return selector

  def createShiftPresetSlider(self):
    """
    Create a slider that will allow to update opacity of the rendering
    cf qSlicerVolumeRenderingPresetComboBox
    """
    shiftSlider = ctk.ctkDoubleSlider()
    shiftSlider.setValue(0.5)
    shiftSlider.singleStep = 0.1
    shiftSlider.pageStep = 0.1
    shiftSlider.maximum = 1.0
    shiftSlider.setOrientation(qt.Qt.Horizontal)
    return shiftSlider

  def createWindowLevelWidget(self):
    """
    Create a widget that will allow to modify the window and level of 2D views
    cf qMRMLWindowLevelWidget
    """
    widget = slicer.qMRMLWindowLevelWidget()
    return widget

  def createSlabThicknessSlider(self):
    """
    Create a slider that will allow to set the thickness of the MIP slab mode.
    """
    slider = qt.QSlider()
    slider.singleStep = 1
    slider.pageStep = 5
    slider.minimum = 1
    slider.maximum = 100
    slider.setOrientation(qt.Qt.Horizontal)
    slider.setValue(1)
    return slider

  def createSlabThicknessSliderReset(self, volx):
    self.slabThicknessSlider.setMaximum(volx)
  def createSlabThicknessButton2(self):
    """
    Create a slider that will allow to set the thickness of the MIP slab mode.
    """
    btn = qt.QPushButton("2")
    btn.setFixedSize(qt.QSize(20,20))
    return btn
  def createSlabThicknessButton5(self):
    """
    Create a slider that will allow to set the thickness of the MIP slab mode.
    """
    btn = qt.QPushButton("5")
    btn.setFixedSize(qt.QSize(20,20))
    return btn
  def createSlabThicknessButton10(self):
    """
    Create a slider that will allow to set the thickness of the MIP slab mode.
    """
    btn = qt.QPushButton("10")
    btn.setFixedSize(qt.QSize(20,20))
    return btn
  def createMIPThicknessCombobox(self):
    """
    Create a combobox to choose the thickness of the MIP
    """
    selector = qt.QComboBox()
    selector.setToolTip(self.tr("Select the thickness of MIP (xx mm)"))
    selector.addItem(self.tr("80 mm"), 80)
    selector.addItem(self.tr("90 mm"), 90)
    selector.addItem(self.tr("100 mm"), 100)
    selector.addItem(self.tr("110 mm"), 110)
    selector.addItem(self.tr("120 mm"), 120)
    selector.addItem(self.tr("130 mm"), 130)
    selector.addItem(self.tr("140 mm"), 140)
    selector.addItem(self.tr("150 mm"), 150)
    selector.addItem(self.tr("160 mm"), 160)
    selector.addItem(self.tr("170 mm"), 170)
    selector.addItem(self.tr("180 mm"), 180)
    selector.setCurrentText(self.tr("80 mm"))
    return selector