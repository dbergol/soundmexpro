object PerformanceForm: TPerformanceForm
  Left = 382
  Top = 232
  BorderStyle = bsSizeToolWin
  Caption = 'Performance'
  ClientHeight = 443
  ClientWidth = 234
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  Position = poScreenCenter
  OnCloseQuery = FormCloseQuery
  TextHeight = 13
  object lv: TListView
    Left = 0
    Top = 0
    Width = 234
    Height = 424
    Align = alClient
    Columns = <
      item
        Caption = 'Name'
        Width = 100
      end
      item
      end
      item
      end>
    TabOrder = 0
    ViewStyle = vsReport
    ExplicitWidth = 242
    ExplicitHeight = 436
  end
  object sb: TStatusBar
    Left = 0
    Top = 424
    Width = 234
    Height = 19
    Panels = <>
    SimplePanel = True
    ExplicitTop = 436
    ExplicitWidth = 242
  end
  object PerformanceTimer: TTimer
    Interval = 300
    OnTimer = PerformanceTimerTimer
    Left = 192
    Top = 32
  end
end
