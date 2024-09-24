object VSTPluginEditor: TVSTPluginEditor
  Left = 582
  Top = 185
  BorderStyle = bsToolWindow
  Caption = 'VSTPluginEditor'
  ClientHeight = 593
  ClientWidth = 588
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
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
  KeyPreview = True
  Menu = mnuMain
  Position = poMainFormCenter
  OnClose = FormClose
  OnDestroy = FormDestroy
  OnShow = FormShow
  TextHeight = 13
  object pcPages: TPageControl
    Left = 0
    Top = 0
    Width = 588
    Height = 593
    ActivePage = tsParameter
    Align = alClient
    TabOrder = 0
    ExplicitWidth = 596
    ExplicitHeight = 605
    object tsParameter: TTabSheet
      Caption = 'tsParameter'
      object scbParameter: TScrollBox
        Left = 0
        Top = 22
        Width = 588
        Height = 555
        HorzScrollBar.Visible = False
        Align = alClient
        BevelOuter = bvNone
        BorderStyle = bsNone
        TabOrder = 0
      end
      object sb: TStatusBar
        Left = 0
        Top = 0
        Width = 588
        Height = 22
        Align = alTop
        Color = clBtnHighlight
        Panels = <
          item
            Text = 'No.'
            Width = 30
          end
          item
            Text = 'Name'
            Width = 150
          end
          item
            Text = 'Value'
            Width = 100
          end
          item
            Text = 'Unit'
            Width = 114
          end
          item
            Width = 100
          end>
      end
    end
    object tsPlugin: TTabSheet
      Caption = 'tsPlugin'
      ImageIndex = 1
    end
  end
  object mnuMain: TMainMenu
    Left = 120
    Top = 80
    object miSettings: TMenuItem
      Caption = 'Settings'
      object miParDlg: TMenuItem
        Caption = 'Parameter Dialog'
        GroupIndex = 1
        RadioItem = True
        OnClick = miDlgClick
      end
      object miPluginDlg: TMenuItem
        Caption = 'Plugin Dialog'
        GroupIndex = 1
        RadioItem = True
        OnClick = miDlgClick
      end
      object N1: TMenuItem
        Caption = '-'
        GroupIndex = 1
      end
      object miUpdatePars: TMenuItem
        Caption = 'Update Parameters'
        GroupIndex = 1
        ShortCut = 116
        OnClick = miUpdateParsClick
      end
    end
    object miPrograms: TMenuItem
      Caption = 'Programs'
    end
  end
end
