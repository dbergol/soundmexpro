object MixerForm: TMixerForm
  Left = 236
  Top = 216
  Width = 676
  Height = 454
  AutoScroll = True
  BorderIcons = [biSystemMenu]
  Caption = 'SoundMexPro Mixer'
  Color = clBtnFace
  Font.Charset = ANSI_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Segoe UI'
  Font.Style = []
  FormStyle = fsStayOnTop
  Icon.Data = {
    0000010001002020100000000000E80200001600000028000000200000004000
    0000010004000000000080020000000000000000000000000000000000000000
    000000008000008000000080800080000000800080008080000080808000C0C0
    C0000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF000000
    0000000000000000000000000000000000000000000000000000000000000077
    777777777777777777777777777000788888888888888888888888888870007F
    F88888888888888888888888F870007FF888888FF88888888FFFFFF88870007F
    F888888FFF888888FFFFFFFFF870007FF88888877F88888887727278F870007F
    F88FFF807F8FFF88822A2228F870007FF888FF807F88F88887AAAAA8F870007F
    F88777807F87778887222228F870007FF88888807F88888887AAAAA8F870007F
    F88FFF807F8FFF88872A2228F870007FF888FF807F88F88887222228F870007F
    F88777807F87778887AAAAA8F870007FF88888807F88888887222228F870007F
    F88FFF807F8FFF88872A2228F870007FF888FF807F88F88887AAAAA8F870007F
    F88777807F87778887222228F870007FF88887000007888887999998F870007F
    F88FF8060008FF8887999998F870007FF888F8877888F88887111118F870007F
    F88777807F87778887999998F870007FF88888807F88888887111118F870007F
    F8888F800788888887111118F870007FF8888887008888888777777FF870007F
    F888888888888888888888888870007FF888888888888888888888888870007F
    FFFFFFFFFFFFFFFFFFFFFFFFFF800078FFFFFFFFFFFFFFFFFFFFFFFFFFF00000
    000000000000000000000000000000000000000000000000000000000000FFFF
    FFFFFFFFFFFFC0000001C0000001C0000001C0000001C0000001C0000001C000
    0001C0000001C0000001C0000001C0000001C0000001C0000001C0000001C000
    0001C0000001C0000001C0000001C0000001C0000001C0000001C0000001C000
    0001C0000001C0000001C0000001C0000001C0000001FFFFFFFFFFFFFFFF}
  KeyPreview = True
  Menu = MainMenu
  OnCanResize = FormCanResize
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  TextHeight = 13
  object ThumbImage: TImage
    Left = 561
    Top = 3
    Width = 15
    Height = 34
    AutoSize = True
    Picture.Data = {
      07544269746D617096060000424D960600000000000036000000280000000F00
      000022000000010018000000000060060000232E0000232E0000000000000000
      00005E514C4E433E382F2C282320201C1A211D1B0B09092C2927211C1A211C1A
      211C1A221D1B221D1B221D1B2823200B0B009C8F8FB6ACABADA4A4ABA2A2ABA2
      A2ABA3A3A29A9AA29A9AAFA7A7ABA2A2ABA1A1AAA0A0AEA6A6888080211D1B17
      1700BFB3B3E6DDDDE4DADAE5DCDCE5DCDCE5DBDBE5DCDCE5DBDBE5DCDCE5DBDB
      E5DBDBE5DBDBE5DCDCBAB1B1211C1A232300BEB3B3EBE4E4E9E0E0EAE1E1E9E1
      E1E9E0E0E9E1E1E9E1E1E9E1E1E9E0E0E9E1E1E9E1E1EBE3E3B6AFAF211C1A2F
      2F00C1B7B7F0EAEAEEE7E7EEE8E8EEE7E7EDE6E6EEE7E7EEE7E7EEE7E7EEE7E7
      EEE7E7EEE7E7EFE9E9BAB4B4211C1A3B3B00C4BBBBF4F1F1F3EEEEF3EEEEF4EF
      EFF2EEEEF3EDEDF3EDEDF3EDEDF4EEEEF4EEEEF3EEEEF3EFEFBFBABA211C1A47
      4700C8C0C0FAF8F8F9F7F7F8F6F6F8F6F6F8F5F5F8F6F6F8F6F6F8F6F6F8F6F6
      F8F5F5F8F5F5FAF7F7C2BFBF221D1B535300CBC5C5FFFFFFFEFFFFFEFEFEFEFE
      FEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFEFFFFFFC5C3C3211C1A5F
      5F00C4B7B7D7C9C9D3C5C5D4C6C6D5C7C7D4C6C6D4C6C6D5C7C7D5C7C7D4C6C6
      D3C5C5D4C6C6D4C6C6B3A9A9221D1B6B6B00B3A5A5DDD2D2DBCECEDBCFCFDBCE
      CEDBCFCFDBCFCFDBCECEDBCECEDCD0D0DBCECEDBCFCFDBCFCFACA1A1221D1B77
      7700C4BBBBF5F2F2F3EDEDF3EFEFF3EFEFF3EFEFF3EFEFF3EEEEF3EFEFF3EEEE
      F4EFEFF3EFEFF5EFEFBFBABA211C1A838300B0A0A0D4C5C5D1C1C1D2C2C2D2C2
      C2D2C3C3D1C2C2D2C3C3D2C3C3D1C2C2D3C4C4D4C4C4D3C3C3A69999211D1B8F
      8F00B3A4A4DACECED9CCCCDACDCDDACCCCD9CCCCD9CCCCDACDCDDACDCDDACECE
      DACDCDD9CCCCDACDCDAA9F9F211C1A9B9B00C9C1C1FCFAFAFBF9F9FBF8F8FBF9
      F9FBF8F8FBF8F8FBF9F9FBF9F9FBF9F9FBF9F9FBF8F8FBF9F9C2BEBE221D1BA7
      A700A79595C6B4B4C6B4B4C7B4B4C5B3B3C6B4B4C6B4B4C5B3B3C6B4B4C5B3B3
      C6B5B5C6B4B4C6B5B59F9090221D1BB3B300BAAFAFE5DBDBE4DADAE4DBDBE3DA
      DAE3DADAE3DADAE3DADAE3DADAE4DBDBE3DADAE3DADAE4DBDBB3AAAA221D1BBF
      BF008771719F87879E85859C84849D85859D84849D85859D85859D85859E8585
      9E85859E85859E86867E6B6B211C1ACBCB005841415A3F3F5A3F3F5B40405A3F
      3F593E3E593E3E5A3F3F5A3F3F5A3F3F593E3E5A3F3F5C41414D3838211D1BD7
      D700D7D1D1EDE9E9EDE9E9EDE9E9EDE8E8EDE9E9EEE8E8EDE8E8EDE9E9EDE8E8
      EDE8E8EDE9E9EDE9E9C6C1C1211D1BE3E300A89797C5B2B2C5B2B2C5B2B2C4B1
      B1C4B1B1C5B2B2C4B1B1C4B1B1C4B1B1C4B0B0C4B1B1C5B1B19B8B8B221E1CEF
      EF00BFB4B4ECE5E5ECE4E4ECE5E5EAE3E3EBE3E3EBE3E3EAE3E3EBE3E3EBE4E4
      EBE2E2EAE2E2EBE4E4B7B0B0211C1AFBFB00C2B7B7F0EBEBEEE8E8F0EAEAEFE9
      E9F0EAEAF0E9E9EFE9E9F0EAEAF0E9E9F1EBEBEFE8E8EEE8E8BBB4B4211C1AFF
      FF8CAB9A9ACCBBBBCAB9B9CBB9B9CBB9B9CBB9B9CAB8B8CBB9B9CBB9B9CAB8B8
      CBB9B9CAB8B8CBB9B99E8F8F211C1AFFFF8CBFB4B4EBE3E3E9E2E2E9E2E2EAE2
      E2E8E1E1E9E2E2EAE2E2E9E2E2E9E1E1E8E0E0EAE2E2E8E1E1B6AEAE211D1BFF
      FF8CC3BABAF6F1F1F4F0F0F4F0F0F5F0F0F4F0F0F4F0F0F4EFEFF5F0F0F5F1F1
      F4F0F0F4F0F0F5F0F0C0BCBC211D1BFFFF8CA99898D0BEBECDBCBCCDBCBCCDBC
      BCCEBDBDCEBDBDCDBCBCCDBCBCCEBCBCCCBBBBCEBCBCCEBDBDA29393211C1AFF
      FF8CB4A7A7DFD3D3DCD1D1DCD1D1DBD0D0DCD1D1DCD1D1DDD1D1DCD1D1DDD2D2
      DCD0D0DBCECEDCD0D0ABA0A0211C1AFFFF8CCAC4C4FFFFFFFEFEFEFEFEFEFDFD
      FDFEFEFEFEFEFEFFFEFEFEFEFEFFFFFFFFFEFEFFFFFFFFFFFFC3C1C1211C1AFF
      FFFDDED8D8FAF8F8F7F4F4F8F5F5F7F4F4F8F5F5F8F5F5F8F5F5F7F4F4F8F5F5
      F8F5F5F8F5F5F9F6F6CDCBCB211C1AFFFF8CC3BABAF4F0F0F2EDEDF2EDEDF3ED
      EDF2EDEDF2EDEDF1ECECF2ECECF1ECECF3EDEDF2EDEDF3EEEEBEB9B9221D1BFF
      FF8CC1B6B6EFEAEAEDE6E6EEE7E7EEE7E7EDE6E6EDE7E7ECE6E6EDE6E6EDE6E6
      EEE7E7EDE7E7EEE7E7BEB7B7282220FFFF8CBFB4B4EBE3E3E9E1E1EAE1E1EAE1
      E1E9E0E0E9E0E0E9E0E0EAE1E1E9E0E0E9E0E0EAE2E2EAE3E3BEB6B6382F2CFF
      FF8CC2B5B5E4DBDBE4DADAE4DADAE4D9D9E3DADAE5DBDBE3DADAE3DADAE4DADA
      E3DBDBE4D9D9E4D9D9C2B7B74E423E00D800AB9B9BC8BBBBC6B9B9C6B8B8C6B7
      B7C9BCBCB6AAAAB7ABABCEC1C1C5B7B7C6B8B8C5B7B7C7B9B9AB9C9C5B4E4800
      288C}
    Visible = False
  end
  object sbStatus: TStatusBar
    Left = 0
    Top = 401
    Width = 660
    Height = 14
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
    ParentShowHint = False
    ShowHint = True
    ExplicitTop = 381
  end
  object pnlTracks: TPanel
    Left = 11
    Top = 0
    Width = 150
    Height = 401
    Align = alLeft
    BevelOuter = bvLowered
    Color = clSkyBlue
    ParentBackground = False
    TabOrder = 1
    StyleElements = []
    ExplicitHeight = 381
  end
  object pnlOutputs: TPanel
    Left = 172
    Top = 0
    Width = 101
    Height = 401
    Align = alLeft
    BevelOuter = bvLowered
    Color = clMoneyGreen
    ParentBackground = False
    TabOrder = 2
    StyleElements = []
    ExplicitHeight = 381
  end
  object pnlInputs: TPanel
    Left = 284
    Top = 0
    Width = 261
    Height = 401
    Align = alLeft
    BevelOuter = bvLowered
    Color = 10930928
    TabOrder = 3
    StyleElements = []
    ExplicitHeight = 381
  end
  object pnlBtnTracks: TPanel
    AlignWithMargins = True
    Left = 0
    Top = 0
    Width = 11
    Height = 400
    Margins.Left = 0
    Margins.Top = 0
    Margins.Right = 0
    Margins.Bottom = 1
    Align = alLeft
    BevelInner = bvRaised
    BevelOuter = bvLowered
    Color = clSkyBlue
    ParentBackground = False
    TabOrder = 4
    StyleElements = []
    OnClick = pnlTracksClick
    ExplicitHeight = 380
  end
  object pnlBtnInputs: TPanel
    Left = 273
    Top = 0
    Width = 11
    Height = 401
    Align = alLeft
    BevelInner = bvRaised
    BevelOuter = bvLowered
    Color = 10930928
    ParentBackground = False
    TabOrder = 5
    StyleElements = []
    OnClick = pnlInputsClick
    ExplicitHeight = 381
  end
  object pnlBtnOutputs: TPanel
    Left = 161
    Top = 0
    Width = 11
    Height = 401
    Align = alLeft
    BevelInner = bvRaised
    BevelOuter = bvLowered
    Color = clMoneyGreen
    ParentBackground = False
    TabOrder = 6
    StyleElements = []
    OnClick = pnlOutputsClick
    ExplicitHeight = 381
  end
  object MainMenu: TMainMenu
    Left = 520
    Top = 8
    object About1: TMenuItem
      Caption = 'About'
      OnClick = About1Click
    end
  end
  object LevelUpdateTimer: TTimer
    Enabled = False
    Interval = 100
    OnTimer = LevelUpdateTimerTimer
    Left = 592
    Top = 56
  end
end