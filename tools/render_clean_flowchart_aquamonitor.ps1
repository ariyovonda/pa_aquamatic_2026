Add-Type -AssemblyName System.Drawing

$out = Join-Path (Get-Location) "docs\flowchart-aquamonitor-hitam-putih.jpg"
$w = 2200
$h = 2950
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::White)

$fontTitle = New-Object System.Drawing.Font("Arial", 42, [System.Drawing.FontStyle]::Bold)
$fontSub = New-Object System.Drawing.Font("Arial", 23, [System.Drawing.FontStyle]::Regular)
$font = New-Object System.Drawing.Font("Arial", 24, [System.Drawing.FontStyle]::Regular)
$fontBold = New-Object System.Drawing.Font("Arial", 24, [System.Drawing.FontStyle]::Bold)
$fontSmall = New-Object System.Drawing.Font("Arial", 20, [System.Drawing.FontStyle]::Regular)

$black = [System.Drawing.Color]::Black
$white = [System.Drawing.Color]::White
$pen = New-Object System.Drawing.Pen($black, 4)
$penArrow = New-Object System.Drawing.Pen($black, 4)
$cap = New-Object System.Drawing.Drawing2D.AdjustableArrowCap(7, 7)
$penArrow.CustomEndCap = $cap
$brushWhite = New-Object System.Drawing.SolidBrush($white)
$brushBlack = New-Object System.Drawing.SolidBrush($black)
$sf = New-Object System.Drawing.StringFormat
$sf.Alignment = [System.Drawing.StringAlignment]::Center
$sf.LineAlignment = [System.Drawing.StringAlignment]::Center

function Draw-CenteredText($text, $rect, $fontObj) {
  $g.DrawString($text, $fontObj, $brushBlack, $rect, $sf)
}

function Draw-Process($x, $y, $ww, $hh, $text) {
  $rect = New-Object System.Drawing.RectangleF($x, $y, $ww, $hh)
  $g.FillRectangle($brushWhite, $rect)
  $g.DrawRectangle($pen, [int]$x, [int]$y, [int]$ww, [int]$hh)
  Draw-CenteredText $text $rect $font
}

function Draw-Terminal($x, $y, $ww, $hh, $text) {
  $rect = New-Object System.Drawing.RectangleF($x, $y, $ww, $hh)
  $path = New-Object System.Drawing.Drawing2D.GraphicsPath
  $r = 34
  $path.AddArc($x, $y, $r, $r, 180, 90)
  $path.AddArc($x + $ww - $r, $y, $r, $r, 270, 90)
  $path.AddArc($x + $ww - $r, $y + $hh - $r, $r, $r, 0, 90)
  $path.AddArc($x, $y + $hh - $r, $r, $r, 90, 90)
  $path.CloseFigure()
  $g.FillPath($brushWhite, $path)
  $g.DrawPath($pen, $path)
  Draw-CenteredText $text $rect $fontBold
}

function Draw-DecisionBox($x, $y, $ww, $hh, $text) {
  $rect = New-Object System.Drawing.RectangleF($x, $y, $ww, $hh)
  $g.FillRectangle($brushWhite, $rect)
  $g.DrawRectangle($pen, [int]$x, [int]$y, [int]$ww, [int]$hh)
  Draw-CenteredText $text $rect $fontBold
}

function Arrow($x1, $y1, $x2, $y2) {
  $g.DrawLine($penArrow, [int]$x1, [int]$y1, [int]$x2, [int]$y2)
}

function Line($x1, $y1, $x2, $y2) {
  $g.DrawLine($pen, [int]$x1, [int]$y1, [int]$x2, [int]$y2)
}

function Label($text, $x, $y, $ww, $hh) {
  $rect = New-Object System.Drawing.RectangleF($x, $y, $ww, $hh)
  $g.DrawString($text, $fontSmall, $brushBlack, $rect, $sf)
}

$titleRect = New-Object System.Drawing.RectangleF(0, 30, $w, 60)
$subRect = New-Object System.Drawing.RectangleF(0, 95, $w, 40)
$g.DrawString("Flowchart Sistem AquaMonitor", $fontTitle, $brushBlack, $titleRect, $sf)
$g.DrawString("Alur monitoring kualitas air dan kendali aktuator akuaponik", $fontSub, $brushBlack, $subRect, $sf)

$cx = 1100
$bw = 620
$bh = 95
$x = $cx - ($bw / 2)
$y0 = 180
$gap = 45

Draw-Terminal $x $y0 $bw $bh "START"
Draw-Process $x ($y0+140) $bw $bh "Inisialisasi sensor, koneksi,`ndan sistem komunikasi"
Draw-Process $x ($y0+280) $bw $bh "Sensor membaca kualitas air`n(suhu, pH, TDS, DO, turbidity)"
Draw-Process $x ($y0+420) $bw $bh "Arduino mengolah data sensor`ndan membentuk frame serial"
Draw-Process $x ($y0+560) $bw $bh "ESP8266 mengirim data`nke MQTT"
Draw-Process $x ($y0+700) $bw $bh "Node-RED menerima, memeriksa,`ndan memproses data"
Draw-Process $x ($y0+840) $bw $bh "Firebase menyimpan data live`ndan histori sensor"
Draw-Process $x ($y0+980) $bw $bh "Dashboard menampilkan data sensor`ndan status aktuator"
Draw-DecisionBox $x ($y0+1140) $bw 115 "Apakah nilai sensor`nmelewati batas?"

Arrow $cx ($y0+$bh) $cx ($y0+140)
Arrow $cx ($y0+140+$bh) $cx ($y0+280)
Arrow $cx ($y0+280+$bh) $cx ($y0+420)
Arrow $cx ($y0+420+$bh) $cx ($y0+560)
Arrow $cx ($y0+560+$bh) $cx ($y0+700)
Arrow $cx ($y0+700+$bh) $cx ($y0+840)
Arrow $cx ($y0+840+$bh) $cx ($y0+980)
Arrow $cx ($y0+980+$bh) $cx ($y0+1140)

$decY = $y0+1140
$rightX = 1660
$leftX = 420
$branchY = $decY+58

Line ($x+$bw) $branchY $rightX $branchY
Arrow $rightX $branchY $rightX ($decY+170)
Label "YA" ($x+$bw+20) ($branchY-36) 80 35

Draw-Process ($rightX-260) ($decY+170) 520 $bh "Node-RED mengirim command`naktuator melalui MQTT"
Draw-Process ($rightX-260) ($decY+310) 520 $bh "ESP8266 meneruskan command`nke Arduino"
Draw-Process ($rightX-260) ($decY+450) 520 $bh "Arduino menjalankan aktuator`ndengan pengaman"
Draw-Process ($rightX-260) ($decY+590) 520 $bh "Status aktual aktuator`ndiperbarui di Firebase"
Draw-Process ($rightX-260) ($decY+730) 520 $bh "Dashboard menampilkan`nstatus perangkat"

Arrow $rightX ($decY+170+$bh) $rightX ($decY+310)
Arrow $rightX ($decY+310+$bh) $rightX ($decY+450)
Arrow $rightX ($decY+450+$bh) $rightX ($decY+590)
Arrow $rightX ($decY+590+$bh) $rightX ($decY+730)

Line $x $branchY $leftX $branchY
Arrow $leftX $branchY $leftX ($decY+170)
Label "TIDAK" ($leftX+25) ($branchY-36) 100 35

Draw-Process ($leftX-260) ($decY+170) 520 $bh "Sistem melanjutkan`nmonitoring"

$joinY = $decY+890
Line $leftX ($decY+170+$bh) $leftX $joinY
Line $rightX ($decY+730+$bh) $rightX $joinY
Line $leftX $joinY $rightX $joinY
Line $cx $joinY $cx ($joinY+70)
Draw-DecisionBox $x ($joinY+70) $bw 105 "Apakah sistem`ndimatikan?"
Arrow $cx $joinY $cx ($joinY+70)

$dec2Y = $joinY+70
Label "YA" ($x+$bw+20) ($dec2Y+30) 80 35
Line ($x+$bw) ($dec2Y+52) $rightX ($dec2Y+52)
Arrow $rightX ($dec2Y+52) $rightX ($dec2Y+190)
Draw-Terminal ($rightX-260) ($dec2Y+190) 520 $bh "END"

Label "TIDAK" ($x-130) ($dec2Y+30) 120 35
Line $x ($dec2Y+52) $leftX ($dec2Y+52)
Line $leftX ($dec2Y+52) $leftX 300
Line $leftX 300 $cx 300
Arrow $cx 300 $cx ($y0+280)

$note = "Catatan: garis alur dibuat horizontal dan vertikal. Aktuator bekerja dengan pengaman pulse, cycle, dosing, dan lockout."
$noteRect = New-Object System.Drawing.RectangleF(310, 2800, 1580, 70)
$g.DrawRectangle($pen, 280, 2785, 1640, 90)
$g.DrawString($note, $fontSmall, $brushBlack, $noteRect, $sf)

$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Jpeg)
$g.Dispose()
$bmp.Dispose()

Write-Host "Rendered $out"
