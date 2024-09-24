object formVersionCheck: TformVersionCheck
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Version Information'
  ClientHeight = 440
  ClientWidth = 624
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poOwnerFormCenter
  DesignSize = (
    624
    440)
  TextHeight = 15
  object lblLatestVersion: TLabel
    AlignWithMargins = True
    Left = 12
    Top = 5
    Width = 609
    Height = 15
    Margins.Left = 12
    Margins.Top = 5
    Align = alTop
    Caption = 'lblLatestVersion'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Segoe UI'
    Font.Style = [fsBold]
    ParentFont = False
    ExplicitWidth = 88
  end
  object lblCurrentVersion: TLabel
    AlignWithMargins = True
    Left = 12
    Top = 24
    Width = 609
    Height = 15
    Margins.Left = 12
    Margins.Top = 1
    Margins.Bottom = 8
    Align = alTop
    Caption = 'lblVersions'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    ExplicitWidth = 56
  end
  object OKButton: TButton
    Left = 499
    Top = 403
    Width = 114
    Height = 32
    Anchors = [akLeft, akBottom]
    Cancel = True
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
    ExplicitTop = 381
  end
  object btnHP: TButton
    Left = 320
    Top = 403
    Width = 169
    Height = 32
    Anchors = [akLeft, akBottom]
    Cancel = True
    Caption = 'Visit Download Page'
    Default = True
    TabOrder = 1
    OnClick = btnHPClick
    ExplicitTop = 381
  end
  object gbVersionHistory: TGroupBox
    AlignWithMargins = True
    Left = 3
    Top = 50
    Width = 618
    Height = 342
    Align = alTop
    Caption = 'Version History'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    TabOrder = 2
    ExplicitLeft = 0
    ExplicitTop = 21
    ExplicitWidth = 624
    object memVersionInfo: TMemo
      AlignWithMargins = True
      Left = 9
      Top = 20
      Width = 600
      Height = 317
      Margins.Left = 7
      Margins.Right = 7
      Align = alClient
      BorderStyle = bsNone
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Segoe UI'
      Font.Style = []
      ParentFont = False
      ScrollBars = ssBoth
      TabOrder = 0
      ExplicitTop = 22
      ExplicitWidth = 604
    end
  end
end
