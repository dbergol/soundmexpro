object frmVisual: TfrmVisual
  Left = 2
  Top = 2
  Caption = 'Visualization'
  ClientHeight = 345
  ClientWidth = 271
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  Icon.Data = {
    0000010001002020100000000000E80200001600000028000000200000004000
    0000010004000000000080020000000000000000000000000000000000000000
    000000008000008000000080800080000000800080008080000080808000C0C0
    C0000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF000000
    0000000000000000000000000000077777777777777777777777777777700F88
    88888888888888888888888888700F87FFFFFFFFFFFFFFFFFFFFFFFFF8700F87
    A0A0A0A0A0A0A0A0A0A0A0A0F8700F87A0A0A0A0A0A0A0A0A0A0A0A0F8700F87
    A0A000A0A0A0A000A000A0A0F8700F87909000009090900090009000F8700F87
    900000000090000000009000F8700F87000000000000000000000000F8700F87
    77777777777777777777777778700F8888888888888888888888888888700F87
    FFFFFFFFFFFFFFFFFFFFFFFFF8700F870A0000000000000000000000F8700F87
    0AA00A00000000A000AA000AF8700F870000000A000A0A00A000A000F8700F87
    000A000A000A0000AA0000A0F8700F870000A0000000A00000000AA0F8700F87
    00000000A000000000000000F8700F87000000000000000000000000F8700F87
    77777777777777777777777778700F8888888888888888888888888888700F87
    FFFFFFFFFFFFFFFFFFFFFFFFF8700F87000000000000000000000000F8700F87
    AAAAAAAAAAAAAAAA99999990F8700F87000000000000000000000000F8700F87
    AAAAAAAAAAAAAAAA99999000F8700F87000000000000000000000000F8700F87
    77777777777777777777777778700F8888888888888888888888888888700FFF
    FFFFFFFFFFFFFFFFFFFFFFFFFFF0000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    0000000000000000000000000000000000000000000000000000000000000000
    000000000000000000000000000000000000000000000000000000000000}
  OldCreateOrder = True
  OnCloseQuery = FormCloseQuery
  OnResize = FormResize
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object gbMain: TGroupBox
    Left = 0
    Top = 0
    Width = 271
    Height = 345
    Align = alClient
    PopupMenu = PopupMenu
    TabOrder = 0
    object Splitter1: TSplitter
      Left = 2
      Top = 30
      Width = 267
      Height = 3
      Cursor = crVSplit
      Align = alTop
      AutoSnap = False
      Beveled = True
      MinSize = 24
      OnMoved = Splitter1Moved
      ExplicitWidth = 275
    end
    object Splitter2: TSplitter
      Left = 2
      Top = 133
      Width = 267
      Height = 3
      Cursor = crVSplit
      Align = alTop
      AutoSnap = False
      Beveled = True
      MinSize = 20
      OnMoved = Splitter2Moved
      ExplicitWidth = 275
    end
    object Splitter3: TSplitter
      Left = 2
      Top = 233
      Width = 267
      Height = 3
      Cursor = crVSplit
      Align = alTop
      AutoSnap = False
      Beveled = True
      MinSize = 20
      OnMoved = Splitter3Moved
      ExplicitWidth = 275
    end
    object pnlLevel: TPanel
      Left = 2
      Top = 15
      Width = 267
      Height = 15
      Align = alTop
      BevelOuter = bvNone
      TabOrder = 0
      OnResize = pnlLevelResize
      object LevelL: TMMLevel
        Left = 0
        Top = 0
        Width = 267
        Height = 7
        Tag2 = 0
        Align = alTop
        Bevel.BevelInner = bvNone
        Bevel.BevelOuter = bvLowered
        Bevel.BevelInnerWidth = 1
        Bevel.BevelOuterWidth = 1
        Bevel.BorderStyle = bsNone
        Bevel.BorderWidth = 0
        Bevel.BorderSpace = 0
        Bevel.BorderColor = clBtnFace
        Bevel.BorderSpaceColor = clBlack
        Bevel.InnerLightColor = clBtnHighlight
        Bevel.InnerShadowColor = clBtnShadow
        Bevel.OuterLightColor = clBtnHighlight
        Bevel.OuterShadowColor = clBtnShadow
        PopupMenu = PopupMenu
        Bar1Color = clLime
        Bar2Color = clYellow
        Inactive1Color = clGreen
        Inactive2Color = clOlive
        Point1 = 80
        BitLength = b16Bit
        Channel = chLeft
        Mode = mStereo
        Sensitivy = -90
        ExplicitWidth = 275
      end
      object LevelScale: TMMLevelScale
        Left = 0
        Top = 7
        Width = 267
        Height = 8
        Align = alTop
        PopupMenu = PopupMenu
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -8
        Font.Name = 'Small Fonts'
        Font.Style = []
        ScaleTicks = 7
        Sensitivy = -90
        Scale.TickCount = 11
        Scale.EnlargeEvery = 5
        Scale.Size = 12
        Scale.Origin = soOuter
        Scale.Connect = False
        ExplicitWidth = 275
      end
    end
    object pnlSpectrum: TPanel
      Left = 2
      Top = 136
      Width = 267
      Height = 97
      Align = alTop
      BevelOuter = bvNone
      Caption = 'pnlSpectrum'
      TabOrder = 1
      object Spectrum: TMMSpectrum
        Left = 0
        Top = 0
        Width = 240
        Height = 97
        Tag2 = 0
        Align = alClient
        Bevel.BevelInner = bvNone
        Bevel.BevelOuter = bvLowered
        Bevel.BevelInnerWidth = 1
        Bevel.BevelOuterWidth = 1
        Bevel.BorderStyle = bsNone
        Bevel.BorderWidth = 0
        Bevel.BorderSpace = 3
        Bevel.BorderColor = clBtnFace
        Bevel.BorderSpaceColor = clBlack
        Bevel.InnerLightColor = clBtnHighlight
        Bevel.InnerShadowColor = clBtnShadow
        Bevel.OuterLightColor = clBtnHighlight
        Bevel.OuterShadowColor = clBtnShadow
        PopupMenu = PopupMenu
        Bar1Color = clLime
        Bar2Color = clYellow
        Inactive1Color = clGreen
        Inactive2Color = clOlive
        Point1 = 80
        DrawInactive = False
        Mode = mStereo
        BitLength = b16Bit
        SampleRate = 44100
        FFTLength = 512
        Window = fwHanning
        DecayMode = dmExponential
        FrequencyScale = 2
        PeakDelay = 0
        PeakSpeed = 3
        ExplicitWidth = 248
      end
      object pnlScopBtns: TPanel
        Left = 240
        Top = 0
        Width = 27
        Height = 97
        Align = alRight
        BevelInner = bvLowered
        BevelOuter = bvNone
        TabOrder = 0
        DesignSize = (
          27
          97)
        object btnSpectrumLog: TSpeedButton
          Left = 1
          Top = 76
          Width = 25
          Height = 19
          AllowAllUp = True
          Anchors = [akLeft, akBottom]
          GroupIndex = 2
          Caption = 'dB'
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -9
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
          Transparent = False
          OnClick = btnSpectroLogClick
        end
        object btnSpectrumLogF: TSpeedButton
          Left = 1
          Top = 57
          Width = 25
          Height = 19
          AllowAllUp = True
          Anchors = [akLeft, akBottom]
          GroupIndex = 1
          Caption = 'ln(f)'
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -9
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
          Transparent = False
          OnClick = btnSpectrumLogFClick
        end
        object btnSpectrumZoomIn: TSpeedButton
          Tag = 1
          Left = 1
          Top = 1
          Width = 25
          Height = 23
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
          OnClick = btnSpectrumZoomInClick
        end
        object btnSpectrumZoomOut: TSpeedButton
          Tag = -1
          Left = 1
          Top = 24
          Width = 25
          Height = 23
          Glyph.Data = {
            76010000424D7601000000000000760000002800000020000000100000000100
            04000000000000010000130B0000130B00001000000000000000000000000000
            800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
            FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
            33033333333333333F7F3333333333333000333333333333F777333333333333
            000333333333333F777333333333333000333333333333F77733333333333300
            033333333FFF3F777333333700073B703333333F7773F77733333307777700B3
            333333773337777333333078F8F87033333337F3333337F33333778F8F8F8773
            333337333333373F333307F8F8F8F70333337F33FFFFF37F3333078999998703
            33337F377777337F333307F8F8F8F703333373F3333333733333778F8F8F8773
            333337F3333337F333333078F8F870333333373FF333F7333333330777770333
            333333773FF77333333333370007333333333333777333333333}
          NumGlyphs = 2
          OnClick = btnSpectrumZoomOutClick
        end
      end
    end
    object pnlSpectro: TPanel
      Left = 2
      Top = 236
      Width = 267
      Height = 107
      Align = alClient
      BevelOuter = bvNone
      Caption = 'pnlSpectro'
      TabOrder = 2
      object Spectrogram: TMMSpectrogram
        Left = 0
        Top = 0
        Width = 240
        Height = 107
        Tag2 = 0
        Align = alClient
        Bevel.BevelInner = bvNone
        Bevel.BevelOuter = bvLowered
        Bevel.BevelInnerWidth = 1
        Bevel.BevelOuterWidth = 1
        Bevel.BorderStyle = bsNone
        Bevel.BorderWidth = 0
        Bevel.BorderSpace = 3
        Bevel.BorderColor = clBtnFace
        Bevel.BorderSpaceColor = clBlack
        Bevel.InnerLightColor = clBtnHighlight
        Bevel.InnerShadowColor = clBtnShadow
        Bevel.OuterLightColor = clBtnHighlight
        Bevel.OuterShadowColor = clBtnShadow
        PopupMenu = PopupMenu
        Accelerate = False
        Mode = mStereo
        BitLength = b16Bit
        SampleRate = 44100
        FFTLength = 512
        Window = fwHanning
        FrequencyScale = 2
        ExplicitWidth = 248
        ExplicitHeight = 118
      end
      object pnlSpectroBtns: TPanel
        Left = 240
        Top = 0
        Width = 27
        Height = 107
        Align = alRight
        BevelInner = bvLowered
        BevelOuter = bvNone
        TabOrder = 0
        DesignSize = (
          27
          107)
        object btnSpectroLog: TSpeedButton
          Left = 1
          Top = 88
          Width = 25
          Height = 19
          AllowAllUp = True
          Anchors = [akLeft, akBottom]
          GroupIndex = 3
          Caption = 'dB'
          Font.Charset = ANSI_CHARSET
          Font.Color = clWindowText
          Font.Height = -9
          Font.Name = 'Tahoma'
          Font.Style = []
          ParentFont = False
          Transparent = False
          OnClick = btnSpectroLogClick
        end
      end
    end
    object pnlWave: TPanel
      Left = 2
      Top = 33
      Width = 267
      Height = 100
      Align = alTop
      BevelOuter = bvNone
      Caption = 'pnlWave'
      TabOrder = 3
      object Oscope: TMMOscope
        Left = 0
        Top = 0
        Width = 240
        Height = 100
        Tag2 = 0
        Align = alClient
        Bevel.BevelInner = bvNone
        Bevel.BevelOuter = bvLowered
        Bevel.BevelInnerWidth = 1
        Bevel.BevelOuterWidth = 1
        Bevel.BorderStyle = bsNone
        Bevel.BorderWidth = 0
        Bevel.BorderSpace = 3
        Bevel.BorderColor = clBtnFace
        Bevel.BorderSpaceColor = clBlack
        Bevel.InnerLightColor = clBtnHighlight
        Bevel.InnerShadowColor = clBtnShadow
        Bevel.OuterLightColor = clBtnHighlight
        Bevel.OuterShadowColor = clBtnShadow
        PopupMenu = PopupMenu
        Kind = okConLines
        ForegroundColor = clLime
        BarWidth = 2
        BitLength = b16Bit
        SampleRate = 44100
        Mode = mStereo
        FFTLength = 4096
        ExplicitWidth = 248
      end
      object pnlOscBtns: TPanel
        Left = 240
        Top = 0
        Width = 27
        Height = 100
        Align = alRight
        BevelInner = bvLowered
        BevelOuter = bvNone
        TabOrder = 0
        object btnWaveZoomIn: TSpeedButton
          Tag = 1
          Left = 1
          Top = 1
          Width = 25
          Height = 23
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
          OnMouseDown = btnWaveZoomInMouseDown
        end
        object btnWaveZoomOut: TSpeedButton
          Tag = -1
          Left = 1
          Top = 24
          Width = 25
          Height = 23
          Glyph.Data = {
            76010000424D7601000000000000760000002800000020000000100000000100
            04000000000000010000130B0000130B00001000000000000000000000000000
            800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
            FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
            33033333333333333F7F3333333333333000333333333333F777333333333333
            000333333333333F777333333333333000333333333333F77733333333333300
            033333333FFF3F777333333700073B703333333F7773F77733333307777700B3
            333333773337777333333078F8F87033333337F3333337F33333778F8F8F8773
            333337333333373F333307F8F8F8F70333337F33FFFFF37F3333078999998703
            33337F377777337F333307F8F8F8F703333373F3333333733333778F8F8F8773
            333337F3333337F333333078F8F870333333373FF333F7333333330777770333
            333333773FF77333333333370007333333333333777333333333}
          NumGlyphs = 2
          OnMouseDown = btnWaveZoomInMouseDown
        end
        object btnWaveHZoomIn: TSpeedButton
          Tag = -1
          Left = 1
          Top = 47
          Width = 25
          Height = 23
          Glyph.Data = {
            76010000424D7601000000000000760000002800000020000000100000000100
            04000000000000010000130B0000130B00001000000010000000000000000000
            800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
            FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
            330333F333333F333F7F3033333303333000337FFFFFF7F3F777000000000033
            000337777777777F777330333333033000333373333337F77733333333333300
            033333333FFF3F777333333700073B703333333F7773F77733333307777700B3
            33333377333777733333307F8F8F7033333337F333F337F3333377F8FCF8F773
            3333373337F3373F3333078F8C8F870333337F33F7FFF37F333307FCCCCCF703
            33337F377777337F3333078F8C8F8703333373F337F33373333377F8FCF8F773
            333337F3373337F33333307F8F8F70333333373FF333F7333333330777770333
            333333773FF77333333333370007333333333333777333333333}
          NumGlyphs = 2
          OnClick = btnWaveHZoomInClick
        end
        object btnWaveHZoomOut: TSpeedButton
          Tag = 1
          Left = 1
          Top = 70
          Width = 25
          Height = 23
          Glyph.Data = {
            76010000424D7601000000000000760000002800000020000000100000000100
            04000000000000010000130B0000130B00001000000010000000000000000000
            800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
            FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
            330333F333333F333F7F3033333303333000337FFFFFF7F3F777000000000033
            000337777777777F777330333333033000333373333337F77733333333333300
            033333333FFF3F777333333700073B703333333F7773F77733333307777700B3
            333333773337777333333078F8F87033333337F3333337F33333778F8F8F8773
            333337333333373F333307F8F8F8F70333337F33FFFFF37F3333078CCCCC8703
            33337F377777337F333307F8F8F8F703333373F3333333733333778F8F8F8773
            333337F3333337F333333078F8F870333333373FF333F7333333330777770333
            333333773FF77333333333370007333333333333777333333333}
          NumGlyphs = 2
          OnClick = btnWaveHZoomInClick
        end
      end
    end
  end
  object PopupMenu: TPopupMenu
    Left = 66
    Top = 90
    object Options1: TMenuItem
      Caption = 'Options'
      OnClick = Options1Click
    end
  end
end
