object frmSoundDllProLoaderSettings: TfrmSoundDllProLoaderSettings
  Left = 525
  Top = 358
  BorderStyle = bsDialog
  BorderWidth = 5
  Caption = 'Settings'
  ClientHeight = 98
  ClientWidth = 368
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  TextHeight = 13
  object btnOk: TButton
    Left = 80
    Top = 72
    Width = 75
    Height = 25
    Caption = 'Ok'
    ModalResult = 1
    TabOrder = 0
  end
  object btnCancel: TButton
    Left = 208
    Top = 72
    Width = 75
    Height = 25
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object gb: TGroupBox
    Left = 0
    Top = 0
    Width = 368
    Height = 62
    Align = alTop
    TabOrder = 2
    object Label1: TLabel
      Left = 8
      Top = 26
      Width = 112
      Height = 13
      Caption = 'Path to SoundDllPro.dll:'
    end
    object edPath: TEdit
      Left = 128
      Top = 22
      Width = 199
      Height = 21
      TabOrder = 0
    end
    object btnSelPath: TButton
      Left = 335
      Top = 22
      Width = 27
      Height = 20
      Caption = '...'
      TabOrder = 1
      OnClick = btnSelPathClick
    end
  end
end
