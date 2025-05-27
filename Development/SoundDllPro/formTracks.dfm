object TracksForm: TTracksForm
  Left = 228
  Top = 220
  HorzScrollBar.Visible = False
  VertScrollBar.Visible = False
  Caption = 'SoundMexPro Tracks'
  ClientHeight = 547
  ClientWidth = 932
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  Icon.Data = {
    0000010001002020100001000400E80200001600000028000000200000004000
    0000010004000000000000000000000000000000000000000000000000000000
    000000008000008000000080800080000000800080008080000080808000C0C0
    C0000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF000000
    0000000000000000000000000000088888888888888888888888888888800F88
    88888888888888888888888888800FFFFFFFFFFFFFFFFFFFFFFFFFFFF8800FF8
    FFFFFFFFFFFFFFFFFFFFFFFFF8800FF880000000000000000000000FF8800FF8
    8000000A000000000000000FF8800FF88000000A00000000A0A0000FF8800FF8
    8000A00A0A000000A0A00A0FF8800FF88000A00A0A000000A0A00A0FF8800FF8
    8000A0AA0A000000A0A00A0FF8800FF88000A0AA0A000000A0A00A0FF8800FF8
    800AA0AA0A00A00AA0A0AA0FF8800FF8800AA0AA0AA0A00AA0A0A0AFF8800FF8
    800AA0AA0AA0A00AA0A0A0AFF8800FF8800A0AA0AAA0AA0A0A0AA0AFF8800FF8
    89999999999999999999999FF8800FF88A0A0AA0A0A0AA0A0A0AA00FF8800FF8
    8A0A0A00A0A0AA0A0A0AA00FF8800FF88A0A0A00A0AA0A0A0A0AA00FF8800FF8
    80A00A00A0AA0AA00A0AA00FF8800FF880A00A00A00A00A00A00A00FF8800FF8
    80A00000A00A00A00A00A00FF8800FF880A00000A00A00A00000A00FF8800FF8
    80A00000A00A00A00000A00FF8800FF880000000000A00000000A00FF8800FF8
    80000000000000000000000FF8800FF8888888888888888888888888F8800FF8
    88888888888888888888888888800FFFFFFFFFFFFFFFFFFFFFFFFFFFFF800FFF
    FFFFFFFFFFFFFFFFFFFFFFFFFFF0000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    000000000000000000000000000000000000000000000000000000000000}
  KeyPreview = True
  Menu = MainMenu
  OnKeyDown = FormKeyDown
  TextHeight = 13
  object TopBevel: TBevel
    Left = 0
    Top = 0
    Width = 932
    Height = 3
    Align = alTop
    Shape = bsTopLine
  end
  object sbStatus: TStatusBar
    Left = 0
    Top = 532
    Width = 932
    Height = 15
    Panels = <
      item
        Width = 100
      end
      item
        Width = 50
      end
      item
        Width = 50
      end>
  end
  object pnlTrackContainer: TPanel
    Left = 97
    Top = 3
    Width = 835
    Height = 513
    Align = alClient
    BevelOuter = bvNone
    Caption = 'pnlTrackContainer'
    Constraints.MinHeight = 200
    Constraints.MinWidth = 200
    TabOrder = 1
    object pnlTracks: TPanel
      Left = 0
      Top = 0
      Width = 819
      Height = 496
      Align = alClient
      BevelOuter = bvLowered
      Color = clBtnShadow
      TabOrder = 0
      OnResize = pnlTracksResize
      object chrtTrack: TChart
        Left = 1
        Top = 1
        Width = 817
        Height = 494
        AllowPanning = pmNone
        BackWall.Brush.Style = bsClear
        BackWall.Brush.Gradient.Direction = gdBottomTop
        BackWall.Brush.Gradient.EndColor = clWhite
        BackWall.Brush.Gradient.StartColor = 15395562
        BackWall.Brush.Gradient.Visible = True
        BackWall.Transparent = False
        Foot.Font.Color = clBlue
        Foot.Font.Name = 'Verdana'
        Gradient.Direction = gdBottomTop
        Gradient.EndColor = clWhite
        Gradient.MidColor = 15395562
        Gradient.StartColor = 15395562
        LeftWall.Color = clLightyellow
        Legend.Font.Name = 'Verdana'
        Legend.Shadow.Transparency = 0
        Legend.Visible = False
        MarginBottom = 0
        MarginLeft = 0
        MarginRight = 0
        MarginTop = 0
        RightWall.Color = clLightyellow
        Title.Font.Name = 'Verdana'
        Title.Text.Strings = (
          'TChart')
        Title.Visible = False
        AxisBehind = False
        BottomAxis.Axis.Color = 4210752
        BottomAxis.Grid.Color = clDarkgray
        BottomAxis.LabelsFormat.Font.Name = 'Verdana'
        BottomAxis.MinorTicks.Visible = False
        BottomAxis.Ticks.Visible = False
        BottomAxis.TicksInner.Color = clDarkgray
        BottomAxis.TicksInner.Visible = False
        BottomAxis.Title.Font.Name = 'Verdana'
        BottomAxis.Visible = False
        DepthAxis.Axis.Color = 4210752
        DepthAxis.Grid.Color = clDarkgray
        DepthAxis.LabelsFormat.Font.Name = 'Verdana'
        DepthAxis.TicksInner.Color = clDarkgray
        DepthAxis.Title.Font.Name = 'Verdana'
        DepthTopAxis.Axis.Color = 4210752
        DepthTopAxis.Grid.Color = clDarkgray
        DepthTopAxis.LabelsFormat.Font.Name = 'Verdana'
        DepthTopAxis.TicksInner.Color = clDarkgray
        DepthTopAxis.Title.Font.Name = 'Verdana'
        Hover.Visible = False
        LeftAxis.Automatic = False
        LeftAxis.AutomaticMaximum = False
        LeftAxis.AutomaticMinimum = False
        LeftAxis.Axis.Color = 4210752
        LeftAxis.Grid.Color = clDarkgray
        LeftAxis.Increment = 1.000000000000000000
        LeftAxis.LabelsFormat.Font.Name = 'Verdana'
        LeftAxis.Maximum = 4.000000000000000000
        LeftAxis.TicksInner.Color = clDarkgray
        LeftAxis.Title.Font.Name = 'Verdana'
        LeftAxis.Visible = False
        RightAxis.Automatic = False
        RightAxis.AutomaticMaximum = False
        RightAxis.AutomaticMinimum = False
        RightAxis.Axis.Color = 4210752
        RightAxis.Grid.Color = clDarkgray
        RightAxis.LabelsFormat.Font.Name = 'Verdana'
        RightAxis.Maximum = 4.000000000000000000
        RightAxis.TicksInner.Color = clDarkgray
        RightAxis.Title.Font.Name = 'Verdana'
        RightAxis.Visible = False
        TopAxis.Automatic = False
        TopAxis.AutomaticMaximum = False
        TopAxis.AutomaticMinimum = False
        TopAxis.Axis.Color = 4210752
        TopAxis.Axis.Visible = False
        TopAxis.AxisValuesFormat = '0'
        TopAxis.DateTimeFormat = 'hh:nn:ss.zzz'
        TopAxis.Grid.Color = clDarkgray
        TopAxis.Grid.Visible = False
        TopAxis.LabelsFormat.Font.Charset = ANSI_CHARSET
        TopAxis.LabelsFormat.Font.Height = -9
        TopAxis.LabelsFormat.Font.Name = 'Tahoma'
        TopAxis.LabelsOnAxis = False
        TopAxis.LabelsSeparation = 50
        TopAxis.LabelStyle = talValue
        TopAxis.Maximum = 1000.000000000000000000
        TopAxis.MinorTicks.Visible = False
        TopAxis.Ticks.Color = clBlack
        TopAxis.TicksInner.Color = clDarkgray
        TopAxis.TicksInner.Visible = False
        TopAxis.Title.Font.Name = 'Verdana'
        View3D = False
        View3DWalls = False
        Zoom.Allow = False
        OnGetAxisLabel = chrtTrackGetAxisLabel
        Align = alClient
        BevelOuter = bvLowered
        PopupMenu = mnuTimeLine
        TabOrder = 0
        OnMouseDown = chrtTrackMouseDown
        DesignSize = (
          817
          494)
        DefaultCanvas = 'TGDIPlusCanvas'
        ColorPaletteIndex = 0
        object pbCursor: TPaintBox
          Left = 0
          Top = 2
          Width = 2
          Height = 490
          Anchors = [akTop, akBottom]
          OnPaint = pbCursorPaint
        end
        object DummySeries: TLineSeries
          HorizAxis = aTopAxis
          Brush.BackColor = clDefault
          Pointer.InflateMargins = True
          Pointer.Style = psRectangle
          XValues.Name = 'X'
          XValues.Order = loAscending
          YValues.Name = 'Y'
          YValues.Order = loNone
        end
        object CursorSeries: TFastLineSeries
          SeriesColor = clRed
          FastPen = True
          LinePen.Color = clRed
          LinePen.Mode = pmNot
          XValues.Name = 'X'
          XValues.Order = loAscending
          YValues.Name = 'Y'
          YValues.Order = loNone
        end
      end
    end
    object pnlVertScroll: TPanel
      Left = 819
      Top = 0
      Width = 16
      Height = 496
      Align = alRight
      BevelOuter = bvNone
      Caption = 'pnlVertScroll'
      TabOrder = 1
      object scbVert: TScrollBar
        Left = 0
        Top = 0
        Width = 16
        Height = 426
        Align = alClient
        Kind = sbVertical
        PageSize = 1
        TabOrder = 0
        TabStop = False
        OnChange = scbVertChange
      end
      object pnlVertBtns: TPanel
        Left = 0
        Top = 426
        Width = 16
        Height = 70
        Align = alBottom
        BevelOuter = bvNone
        TabOrder = 1
        object btnVZoomIn: TSpeedButton
          Left = 0
          Top = 2
          Width = 16
          Height = 23
          Caption = #59555
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe MDL2 Assets'
          Font.Style = []
          Layout = blGlyphBottom
          ParentFont = False
          OnClick = btnVZoomInClick
        end
        object btnVZoomOut: TSpeedButton
          Left = 0
          Top = 24
          Width = 16
          Height = 23
          Caption = #59167
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe MDL2 Assets'
          Font.Style = []
          Layout = blGlyphBottom
          ParentFont = False
          OnClick = btnVZoomOutClick
        end
        object btnVZoomAll: TSpeedButton
          Left = 0
          Top = 46
          Width = 16
          Height = 23
          Caption = #59166
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe MDL2 Assets'
          Font.Style = []
          Layout = blGlyphBottom
          ParentFont = False
          OnClick = btnVZoomAllClick
        end
      end
    end
    object pnlHorzScroll: TPanel
      Left = 0
      Top = 496
      Width = 835
      Height = 17
      Align = alBottom
      BevelOuter = bvNone
      TabOrder = 2
      object scbHorz: TScrollBar
        Left = 0
        Top = 0
        Width = 753
        Height = 17
        Align = alClient
        PageSize = 0
        TabOrder = 0
        TabStop = False
        OnChange = scbHorzChange
      end
      object pnlHorzBtns: TPanel
        Left = 753
        Top = 0
        Width = 82
        Height = 17
        Align = alRight
        BevelOuter = bvNone
        TabOrder = 1
        object btnHZoomIn: TSpeedButton
          Left = 2
          Top = 1
          Width = 23
          Height = 16
          Caption = #59555
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe MDL2 Assets'
          Font.Style = []
          Layout = blGlyphBottom
          ParentFont = False
          OnClick = btnHZoomInClick
        end
        object btnHZoomOut: TSpeedButton
          Left = 24
          Top = 1
          Width = 23
          Height = 16
          Caption = #59167
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe MDL2 Assets'
          Font.Style = []
          Layout = blGlyphBottom
          ParentFont = False
          OnClick = btnHZoomOutClick
        end
        object btnHZoomAll: TSpeedButton
          Left = 46
          Top = 1
          Width = 23
          Height = 16
          Caption = #59166
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe MDL2 Assets'
          Font.Style = []
          Layout = blGlyphBottom
          ParentFont = False
          OnClick = btnHZoomAllClick
        end
      end
    end
  end
  object pnlLeft: TPanel
    Left = 0
    Top = 3
    Width = 97
    Height = 513
    Align = alLeft
    BevelOuter = bvNone
    TabOrder = 2
    object pnlTrackInfo: TPanel
      Left = 0
      Top = 16
      Width = 97
      Height = 497
      Align = alLeft
      BevelOuter = bvLowered
      TabOrder = 0
      object pnlBottomLeft: TPanel
        Left = 1
        Top = 480
        Width = 95
        Height = 16
        Align = alBottom
        BevelOuter = bvNone
        TabOrder = 0
      end
    end
    object pnlTopLeft: TPanel
      Left = 0
      Top = 0
      Width = 97
      Height = 16
      Align = alTop
      BevelOuter = bvNone
      TabOrder = 1
    end
  end
  object pnlBottom: TPanel
    Left = 0
    Top = 516
    Width = 932
    Height = 16
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 3
  end
  object mnuTimeLine: TPopupMenu
    Left = 136
    Top = 104
    object miTime: TMenuItem
      Caption = 'Time'
      GroupIndex = 1
      RadioItem = True
      OnClick = miTimeClick
    end
    object miSamples: TMenuItem
      Caption = 'Samples'
      GroupIndex = 1
      RadioItem = True
      OnClick = miSamplesClick
    end
  end
  object MainMenu: TMainMenu
    Left = 424
    Top = 256
    object miView: TMenuItem
      Caption = 'View'
      object miRefresh: TMenuItem
        Caption = 'Refresh'
        OnClick = miRefreshClick
      end
      object miDisplayFormat: TMenuItem
        Caption = 'Display Format'
        object Time1: TMenuItem
          Caption = 'Time'
          OnClick = miTimeClick
        end
        object Samples1: TMenuItem
          Caption = 'Samples'
          OnClick = miSamplesClick
        end
      end
    end
    object miAbout: TMenuItem
      Caption = 'About'
      OnClick = miAboutClick
    end
  end
  object CursorTimer: TTimer
    Enabled = False
    Interval = 10
    OnTimer = CursorTimerTimer
    Left = 752
    Top = 456
  end
end
