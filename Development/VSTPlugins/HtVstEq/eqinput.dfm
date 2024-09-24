object frmEqInput: TfrmEqInput
  Left = 385
  Top = 171
  Caption = 'Equalizer'
  ClientHeight = 498
  ClientWidth = 691
  Color = clBtnFace
  Constraints.MinHeight = 400
  Constraints.MinWidth = 530
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Icon.Data = {
    0000010001002020100000000000E80200001600000028000000200000004000
    0000010004000000000000020000000000000000000000000000000000000000
    000000008000008000000080800080000000800080008080000080808000C0C0
    C0000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF007777
    7777777777777777777777777777777777777777777777777777777777777700
    0000000000000000000000000077770000000000000200000000000000777700
    00000000000A00000000000000777700A0000000000A00000000000000777700
    A0000000000A20000000000000777700A0000000000A200A0000000000777700
    A0000000000A202A000A000000777700A2000A00002AA02A000A000000777702
    A2000A00002AA0AA000A000000777702AA002A2000AAA0AA002A20000077770A
    AA002A2000AAA0AA002A200A0077770A2A002A2002A2A2AA20AAA02A2077770A
    2A20A2A002A2A2AA20A2A0AAA07777FFFFFFFFFFFFFFFFFFFFFFFFFFFF777700
    0A2A20AA22A2AAA2AAA0AAA0007777000A2A202A22A02AA2AAA02A2000777700
    0AAA002A2AA02AA0AA200A000077770002A2000A2A200A20AA200A0000777700
    02A20002AA200A20AA0000000077770002A20002AA200A00A200000000777700
    00A00002AA200000A20000000077770000A00002AA000000A000000000777700
    00A00000A2000000000000000077770000000000A20000000000000000777700
    00000000A00000000000000000777787784444444444444444F888788877778F
    F84444444444444444F8087808777788884444444444444444F8887888777777
    7777777777777777777777777777777777777777777777777777777777770000
    0000000000000000000000000000000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    000000000000000000000000000000000000000000000000000000000000}
  KeyPreview = True
  Position = poScreenCenter
  Scaled = False
  TextHeight = 13
  object Chart: TChart
    Left = 0
    Top = 29
    Width = 691
    Height = 450
    AllowPanning = pmNone
    BackWall.Color = clBlack
    BackWall.Transparent = False
    BackWall.Picture.Data = {
      07544269746D617066000000424D660000000000000036000000280000000400
      000004000000010018000000000030000000120B0000120B0000000000000000
      0000280000280000280000280000280000280000280000280000280000280000
      280000280000280000280000280000280000}
    Border.Visible = True
    Foot.Alignment = taLeftJustify
    Foot.CustomPosition = True
    Foot.Font.Color = clBlack
    Foot.Font.Style = []
    Foot.Left = 20
    Foot.Text.Strings = (
      'Position:')
    Foot.Top = 416
    Legend.Alignment = laTop
    Legend.Color = clBlack
    Legend.Font.Color = clWhite
    Legend.LegendStyle = lsSeries
    Legend.MaxNumRows = 2
    Legend.Shadow.Color = clGray
    Legend.TextStyle = ltsPlain
    Legend.TopPos = 9
    Legend.VertMargin = 3
    MarginLeft = 6
    MarginRight = 5
    MarginTop = 0
    Title.Alignment = taLeftJustify
    Title.Font.Color = clBlack
    Title.Font.Height = -16
    Title.Font.Style = [fsBold]
    Title.Text.Strings = (
      '')
    BottomAxis.Automatic = False
    BottomAxis.AutomaticMaximum = False
    BottomAxis.AutomaticMinimum = False
    BottomAxis.Grid.Color = clGreen
    BottomAxis.Maximum = 22050.000000000000000000
    BottomAxis.Title.Caption = 'Frequency [Hz]'
    BottomAxis.Title.Font.Height = -16
    Hover.Visible = False
    LeftAxis.Automatic = False
    LeftAxis.AutomaticMaximum = False
    LeftAxis.AutomaticMinimum = False
    LeftAxis.AxisValuesFormat = '#,##0.#############'
    LeftAxis.ExactDateTime = False
    LeftAxis.Grid.Color = clGreen
    LeftAxis.LabelsOnAxis = False
    LeftAxis.LabelsSize = 20
    LeftAxis.Maximum = 40.000000000000000000
    LeftAxis.Minimum = -140.000000000000000000
    LeftAxis.MinorTickCount = 1
    LeftAxis.MinorTicks.Visible = False
    LeftAxis.Ticks.Style = psDash
    LeftAxis.Ticks.Visible = False
    LeftAxis.TicksInner.Visible = False
    LeftAxis.Title.Caption = 'Level [dB]'
    LeftAxis.Title.Font.Height = -16
    LeftAxis.TitleSize = 12
    RightAxis.Automatic = False
    RightAxis.AutomaticMaximum = False
    RightAxis.AutomaticMinimum = False
    RightAxis.ExactDateTime = False
    RightAxis.Logarithmic = True
    RightAxis.Maximum = 100.000000000000000000
    RightAxis.Minimum = 0.000000100000000000
    RightAxis.Title.Angle = 90
    RightAxis.Title.Caption = 'Level dB'
    RightAxis.Visible = False
    TopAxis.Automatic = False
    TopAxis.AutomaticMaximum = False
    TopAxis.AutomaticMinimum = False
    TopAxis.ExactDateTime = False
    TopAxis.Grid.Color = clGreen
    TopAxis.Increment = 1.000000000000000000
    TopAxis.LabelsOnAxis = False
    TopAxis.LabelsSeparation = 1
    TopAxis.LabelStyle = talText
    TopAxis.Maximum = 10000.000000000000000000
    TopAxis.Minimum = 125.000000000000000000
    TopAxis.MinorTickCount = 0
    TopAxis.MinorTickLength = 1
    TopAxis.RoundFirstLabel = False
    TopAxis.Ticks.Style = psDash
    TopAxis.Title.Caption = 'Frequency [Hz]'
    TopAxis.Title.Font.Height = -13
    TopAxis.Visible = False
    View3D = False
    View3DWalls = False
    Zoom.Allow = False
    Zoom.Animated = True
    OnBeforeDrawAxes = ChartBeforeDrawAxes
    Align = alClient
    ParentShowHint = False
    ShowHint = True
    TabOrder = 0
    OnMouseDown = ChartMouseDown
    OnMouseMove = ChartMouseMove
    OnMouseUp = ChartMouseUp
    ExplicitWidth = 687
    ExplicitHeight = 449
    DesignSize = (
      691
      450)
    DefaultCanvas = 'TTeeCanvas3D'
    PrintMargins = (
      15
      17
      15
      17)
    ColorPaletteIndex = 0
    object cbLogFreq: TCheckBox
      Left = 588
      Top = 420
      Width = 65
      Height = 17
      Anchors = [akRight, akBottom]
      Caption = 'Log Axis'
      TabOrder = 0
      OnClick = cbLogFreqClick
      ExplicitLeft = 584
      ExplicitTop = 419
    end
    object FilterSeriesL: TFastLineSeries
      SeriesColor = clBlue
      Title = 'Filter Left'
      VertAxis = aRightAxis
      LinePen.Color = clBlue
      LinePen.Width = 2
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object FilterSeriesR: TFastLineSeries
      Title = 'Filter Right'
      VertAxis = aRightAxis
      LinePen.Color = clRed
      LinePen.Width = 2
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object PreSpecSeriesL: TFastLineSeries
      SeriesColor = clAqua
      Title = 'Input Left'
      VertAxis = aRightAxis
      LinePen.Color = clAqua
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object PreSpecSeriesR: TFastLineSeries
      SeriesColor = 8388863
      Title = 'Input Right'
      VertAxis = aRightAxis
      LinePen.Color = 8388863
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object PostSpecSeriesL: TFastLineSeries
      SeriesColor = clLime
      Title = 'Ouput Left'
      VertAxis = aRightAxis
      LinePen.Color = clLime
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object PostSpecSeriesR: TFastLineSeries
      SeriesColor = clYellow
      Title = 'Output Right'
      VertAxis = aRightAxis
      LinePen.Color = clYellow
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object FileFilterL: TFastLineSeries
      SeriesColor = 16777088
      Title = 'FileFilterLeft'
      VertAxis = aRightAxis
      LinePen.Color = 16777088
      LinePen.Width = 2
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object FileFilterR: TFastLineSeries
      SeriesColor = 8421631
      Title = 'FileFilterRight'
      VertAxis = aRightAxis
      LinePen.Color = 8421631
      LinePen.Width = 2
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object Series1: TLineSeries
      Legend.Visible = False
      ShowInLegend = False
      Brush.BackColor = clDefault
      Pointer.InflateMargins = True
      Pointer.Style = psRectangle
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object DebugSeries: TPointSeries
      Active = False
      VertAxis = aRightAxis
      ClickableLine = False
      Pointer.InflateMargins = True
      Pointer.Style = psCircle
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 691
    Height = 29
    Align = alTop
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 1
    ExplicitWidth = 687
    object btnPaint: TSpeedButton
      Left = 170
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Draw Filter'
      GroupIndex = 1
      Down = True
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000120B0000120B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00337333733373
        3373337F3F7F3F7F3F7F33737373737373733F7F7F7F7F7F7F7F770000000000
        000077777777777777773303333333333333337FFF333333333F370993333333
        3399377773F33333337733033933333339333F7FF7FFFFFFF7FF770777977777
        977777777777777777773303339333339333337F3373F3337333370333393339
        3333377F33373FF7333333033333999333333F7FFFFF777FFFFF770777777777
        777777777777777777773303333333333333337F333333333333370333333333
        3333377F33333333333333033333333333333F7FFFFFFFFFFFFF770777777777
        7777777777777777777733333333333333333333333333333333}
      NumGlyphs = 2
      ParentShowHint = False
      ShowHint = True
      OnClick = btnPaintClick
    end
    object btnZoom: TSpeedButton
      Left = 222
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Zooming'
      GroupIndex = 1
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000130B0000130B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
        33033333333333333F7F3333333333333000333333333333F777333333333333
        000333333333333F777333333333333000333333333333F77733333333333300
        033333333FFF3F777333333700073B703333333F7773F77733333307777700B3
        33333377333777733333307F8F8F7033333337F333F337F3333377F8F9F8F773
        3333373337F3373F3333078F898F870333337F33F7FFF37F333307F99999F703
        33337F377777337F3333078F898F8703333373F337F33373333377F8F9F8F773
        333337F3373337F33333307F8F8F70333333373FF333F7333333330777770333
        333333773FF77333333333370007333333333333777333333333}
      NumGlyphs = 2
      ParentShowHint = False
      ShowHint = True
      OnClick = btnZoomClick
    end
    object btnSpec: TSpeedButton
      Left = 340
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Draw Spectra'
      AllowAllUp = True
      GroupIndex = 2
      Down = True
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000120B0000120B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00337333733373
        3373337F3F7F3F7F3F7F33737373737373733F7F7F7F7F7F7F7F770000000000
        000077777777777777773303333333333333337FF333333F33333709333333C3
        333337773F3FF373F333330393993C3C33333F7F7F77F7F7FFFF77079797977C
        77777777777777777777330339339333C333337FF73373F37F33370C333C3933
        933337773F3737F37FF33303C3C33939C9333F7F7F7FF7F777FF7707C7C77797
        7C97777777777777777733033C3333333C33337F37F33333373F37033C333333
        33C3377F37333333337333033333333333333F7FFFFFFFFFFFFF770777777777
        7777777777777777777733333333333333333333333333333333}
      NumGlyphs = 2
      ParentShowHint = False
      ShowHint = True
      OnClick = btnSpecClick
    end
    object btnFlat: TSpeedButton
      Left = 274
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Flatten Filters'
      Caption = 'Flat'
      ParentShowHint = False
      ShowHint = True
      OnClick = btnFlatClick
    end
    object btnUndoZoom: TSpeedButton
      Left = 248
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Unzoom'
      Caption = '1:1'
      ParentShowHint = False
      ShowHint = True
      OnClick = btnUndoZoomClick
    end
    object btnLeft: TSpeedButton
      Left = 366
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Left Channel'
      AllowAllUp = True
      GroupIndex = 9
      Down = True
      Caption = 'L'
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlue
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnLeftClick
    end
    object btnRight: TSpeedButton
      Left = 392
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Right Channel'
      AllowAllUp = True
      GroupIndex = 10
      Down = True
      Caption = 'R'
      Font.Charset = ANSI_CHARSET
      Font.Color = clRed
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnRightClick
    end
    object btnShift: TSpeedButton
      Left = 196
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Shift Filter'
      GroupIndex = 1
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000120B0000120B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00337333733373
        3373337F3F7F3F7F3F7F33737373737373733F7F7F7F7F7F7F7F770000000000
        00007777777777777777330333333C333333337FFF3337F3333F370993333C33
        3399377773F337F33377330339333C3339333F7FF7FFF7FFF7FF770777977C77
        97777777777777777777330333933C339333337F3373F7F37333370333393C39
        3333377F333737F7333333033333999333333F7FFFFF777FFFFF770777777C77
        77777777777777777777330333333C330333337F333337FF7FF3370333333C00
        003C377F333337777737330333333C3303333F7FFFFFF7FF7FFF770777777777
        7777777777777777777733333333333333333333333333333333}
      NumGlyphs = 2
      ParentShowHint = False
      ShowHint = True
      OnClick = btnPaintClick
    end
    object btnFast: TSpeedButton
      Left = 470
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Fast Spectra Update'
      AllowAllUp = True
      GroupIndex = 6
      Caption = 'Fast'
      ParentShowHint = False
      ShowHint = True
      OnClick = btnFastClick
    end
    object btnInput: TSpeedButton
      Left = 418
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Left Channel'
      AllowAllUp = True
      GroupIndex = 11
      Down = True
      Caption = 'In'
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnInputClick
    end
    object btnOutput: TSpeedButton
      Left = 444
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Right Channel'
      AllowAllUp = True
      GroupIndex = 12
      Down = True
      Caption = 'Out'
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnOutputClick
    end
    object btnLoad: TSpeedButton
      Left = 3
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Load Filter'
      AllowAllUp = True
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000120B0000120B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00555555555555
        5555555555555555555555555555555555555555555555555555555555555555
        555555555555555555555555555555555555555FFFFFFFFFF555550000000000
        55555577777777775F55500B8B8B8B8B05555775F555555575F550F0B8B8B8B8
        B05557F75F555555575F50BF0B8B8B8B8B0557F575FFFFFFFF7F50FBF0000000
        000557F557777777777550BFBFBFBFB0555557F555555557F55550FBFBFBFBF0
        555557F555555FF7555550BFBFBF00055555575F555577755555550BFBF05555
        55555575FFF75555555555700007555555555557777555555555555555555555
        5555555555555555555555555555555555555555555555555555}
      NumGlyphs = 2
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnLoadClick
    end
    object btnSave: TSpeedButton
      Left = 29
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Save Filter'
      AllowAllUp = True
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000120B0000120B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
        333333FFFFFFFFFFFFF33000077777770033377777777777773F000007888888
        00037F3337F3FF37F37F00000780088800037F3337F77F37F37F000007800888
        00037F3337F77FF7F37F00000788888800037F3337777777337F000000000000
        00037F3FFFFFFFFFFF7F00000000000000037F77777777777F7F000FFFFFFFFF
        00037F7F333333337F7F000FFFFFFFFF00037F7F333333337F7F000FFFFFFFFF
        00037F7F333333337F7F000FFFFFFFFF00037F7F333333337F7F000FFFFFFFFF
        00037F7F333333337F7F000FFFFFFFFF07037F7F33333333777F000FFFFFFFFF
        0003737FFFFFFFFF7F7330099999999900333777777777777733}
      NumGlyphs = 2
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnSaveClick
    end
    object btnSkip: TSpeedButton
      Left = 75
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Bypass'
      AllowAllUp = True
      GroupIndex = 100
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000130B0000130B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
        3333333333FFFFF3333333333999993333333333F77777FFF333333999999999
        3333333777333777FF3333993333339993333377FF3333377FF3399993333339
        993337777FF3333377F3393999333333993337F777FF333337FF993399933333
        399377F3777FF333377F993339993333399377F33777FF33377F993333999333
        399377F333777FF3377F993333399933399377F3333777FF377F993333339993
        399377FF3333777FF7733993333339993933373FF3333777F7F3399933333399
        99333773FF3333777733339993333339933333773FFFFFF77333333999999999
        3333333777333777333333333999993333333333377777333333}
      NumGlyphs = 2
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnSkipClick
    end
    object btnMute: TSpeedButton
      Left = 101
      Top = 2
      Width = 25
      Height = 25
      Hint = 'Mute'
      AllowAllUp = True
      GroupIndex = 101
      Font.Charset = ANSI_CHARSET
      Font.Color = clBlack
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = []
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000130B0000130B00001000000010000000000000000000
        8000008000000080800080000000800080008080000080808000C0C0C0000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333303
        3333333333333337333339933333300333333773333333773333399933330003
        3333377333333777333333999330030333333377333377373333333999003303
        3333333777777337333333309993330333333333773733373333333039993303
        3333333377773337333333303399930333333333737733373333333033099903
        3333333373377337333333303303999333333333733777373333333033033999
        3333333373373777333333300003339993333333777733773333333333003309
        9933333333377337733333333330030399933333333377377733333333330003
        3999333333333777377733333333300333993333333333773377}
      NumGlyphs = 2
      ParentFont = False
      ParentShowHint = False
      ShowHint = True
      OnClick = btnMuteClick
    end
  end
  object sb: TStatusBar
    Left = 0
    Top = 479
    Width = 691
    Height = 19
    Panels = <
      item
        Width = 200
      end
      item
        Width = 50
      end>
    ExplicitTop = 478
    ExplicitWidth = 687
  end
  object od: TOpenDialog
    Filter = 'Filter Files|*.flt|All Files|*.*'
    FilterIndex = 0
    Options = [ofHideReadOnly, ofNoChangeDir, ofEnableSizing]
    Left = 8
    Top = 376
  end
  object sd: TSaveDialog
    DefaultExt = 'flt'
    Filter = 'Filter Files|*.flt'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofNoChangeDir, ofEnableSizing]
    Left = 40
    Top = 392
  end
  object UpdateTimer: TTimer
    Interval = 50
    OnTimer = UpdateTimerTimer
    Left = 16
    Top = 469
  end
end
