object frameVSTParam: TframeVSTParam
  Left = 0
  Top = 0
  Width = 562
  Height = 23
  ParentShowHint = False
  ShowHint = True
  TabOrder = 0
  object sb: TStatusBar
    Left = 0
    Top = 0
    Width = 562
    Height = 22
    Hint = 'Hallo '
    Align = alTop
    Panels = <
      item
        Width = 30
      end
      item
        Width = 150
      end
      item
        Width = 100
      end
      item
        Width = 114
      end
      item
        Width = 100
      end>
  end
  object pb: TProgressBar
    Left = 400
    Top = 7
    Width = 158
    Height = 10
    Smooth = True
    TabOrder = 1
    OnMouseDown = pbMouseDown
    OnMouseMove = pbMouseMove
  end
end
