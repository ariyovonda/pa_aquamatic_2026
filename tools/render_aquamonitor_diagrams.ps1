$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$outDir = Join-Path (Get-Location) "docs"

function New-Bitmap($w, $h) {
  $bmp = New-Object System.Drawing.Bitmap($w, $h)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
  $g.Clear([System.Drawing.Color]::White)
  return @($bmp, $g)
}

function Font($size, $style = "Regular") {
  return New-Object System.Drawing.Font("Arial", $size, [System.Drawing.FontStyle]::$style)
}

function Brush($hex) {
  return New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml($hex))
}

function Pen($hex, $width = 2) {
  return New-Object System.Drawing.Pen([System.Drawing.ColorTranslator]::FromHtml($hex), $width)
}

function Draw-CenterText($g, $lines, $x, $y, $w, $h, $size = 16, $bold = $false, $color = "#0f172a") {
  $style = if ($bold) { "Bold" } else { "Regular" }
  $font = Font $size $style
  $brush = Brush $color
  $fmt = New-Object System.Drawing.StringFormat
  $fmt.Alignment = [System.Drawing.StringAlignment]::Center
  $fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
  $text = ($lines -join "`n")
  $rect = New-Object System.Drawing.RectangleF($x, $y, $w, $h)
  $g.DrawString($text, $font, $brush, $rect, $fmt)
  $font.Dispose(); $brush.Dispose(); $fmt.Dispose()
}

function Draw-Text($g, $text, $x, $y, $size = 16, $bold = $false, $color = "#0f172a") {
  $font = Font $size $(if ($bold) { "Bold" } else { "Regular" })
  $brush = Brush $color
  $g.DrawString($text, $font, $brush, [float]$x, [float]$y)
  $font.Dispose(); $brush.Dispose()
}

function RoundedPath($x, $y, $w, $h, $r) {
  $path = New-Object System.Drawing.Drawing2D.GraphicsPath
  $d = $r * 2
  $path.AddArc($x, $y, $d, $d, 180, 90)
  $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
  $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
  $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
  $path.CloseFigure()
  return $path
}

function Draw-Box($g, $x, $y, $w, $h, $fill, $stroke, $lines, $size = 16, $bold = $false) {
  $path = RoundedPath $x $y $w $h 18
  $b = Brush $fill
  $p = Pen $stroke 2.2
  $g.FillPath($b, $path)
  $g.DrawPath($p, $path)
  Draw-CenterText $g $lines $x $y $w $h $size $bold
  $b.Dispose(); $p.Dispose(); $path.Dispose()
}

function Draw-Diamond($g, $x, $y, $w, $h, $fill, $stroke, $lines) {
  $pts = @(
    (New-Object System.Drawing.PointF(($x + $w/2), $y)),
    (New-Object System.Drawing.PointF(($x + $w), ($y + $h/2))),
    (New-Object System.Drawing.PointF(($x + $w/2), ($y + $h))),
    (New-Object System.Drawing.PointF($x, ($y + $h/2)))
  )
  $b = Brush $fill; $p = Pen $stroke 2.2
  $g.FillPolygon($b, $pts); $g.DrawPolygon($p, $pts)
  Draw-CenterText $g $lines $x $y $w $h 16 $false
  $b.Dispose(); $p.Dispose()
}

function Draw-Arrow($g, $x1, $y1, $x2, $y2, $color = "#0f172a", $width = 2.4) {
  $p = Pen $color $width
  $cap = New-Object System.Drawing.Drawing2D.AdjustableArrowCap(5, 5, $true)
  $p.CustomEndCap = $cap
  $g.DrawLine($p, [float]$x1, [float]$y1, [float]$x2, [float]$y2)
  $cap.Dispose(); $p.Dispose()
}

function Draw-PolylineArrow($g, $points, $color = "#0f172a") {
  if ($points.Count -lt 2) { return }
  $p = Pen $color 2.4
  for ($i=0; $i -lt $points.Count-2; $i++) {
    $g.DrawLine($p, [float]$points[$i][0], [float]$points[$i][1], [float]$points[$i+1][0], [float]$points[$i+1][1])
  }
  $cap = New-Object System.Drawing.Drawing2D.AdjustableArrowCap(5, 5, $true)
  $p.CustomEndCap = $cap
  $a = $points[$points.Count-2]; $b = $points[$points.Count-1]
  $g.DrawLine($p, [float]$a[0], [float]$a[1], [float]$b[0], [float]$b[1])
  $cap.Dispose(); $p.Dispose()
}

function Draw-Group($g, $x, $y, $w, $h, $title) {
  $p = Pen "#94a3b8" 2
  $p.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash
  $path = RoundedPath $x $y $w $h 24
  $g.DrawPath($p, $path)
  Draw-Text $g $title ($x+18) ($y+14) 16 $true "#334155"
  $p.Dispose(); $path.Dispose()
}

function Save-Jpg($bmp, $path) {
  $enc = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
  $params = New-Object System.Drawing.Imaging.EncoderParameters(1)
  $params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, 95L)
  $bmp.Save($path, $enc, $params)
}

# Flowchart
$pair = New-Bitmap 1800 1450; $bmp = $pair[0]; $g = $pair[1]
Draw-CenterText $g @("Flowchart Keseluruhan Sistem AquaMonitor") 0 25 1800 40 28 $true
Draw-CenterText $g @("Monitoring kualitas air, Firebase RTDB, threshold user, Node-RED, MQTT, ESP8266, dan kontrol aktuator aman") 0 68 1800 30 16 $false "#475569"

Draw-Group $g 55 120 1650 130 "A. Inisialisasi Sistem"
Draw-Box $g 105 160 170 60 "#dcfce7" "#16a34a" @("START") 18 $true
Draw-Box $g 335 150 280 80 "#e0f2fe" "#0284c7" @("Inisialisasi Arduino","sensor, relay, serial ESP") 15
Draw-Box $g 675 150 280 80 "#e0f2fe" "#0284c7" @("Inisialisasi ESP8266","Wi-Fi dan MQTT broker") 15
Draw-Box $g 1015 150 280 80 "#fef3c7" "#d97706" @("Node-RED Aktif","flow sensor dan aktuator") 15
Draw-Box $g 1355 150 280 80 "#fef3c7" "#d97706" @("Firebase & Dashboard","RTDB + AquaMonitor") 15
Draw-Arrow $g 275 190 335 190; Draw-Arrow $g 615 190 675 190; Draw-Arrow $g 955 190 1015 190; Draw-Arrow $g 1295 190 1355 190

Draw-Group $g 55 285 760 965 "B. Alur Monitoring Data Sensor"
Draw-Box $g 110 345 270 88 "#e0f2fe" "#0284c7" @("Baca Sensor Air","suhu, pH, TDS, DO, turbidity","Arduino Uno") 15
Draw-Box $g 110 485 270 92 "#e0f2fe" "#0284c7" @("Filter & Kalibrasi Awal","pH rolling average","TDS smoothing + kompensasi suhu") 15
Draw-Box $g 110 630 270 90 "#f1f5f9" "#475569" @("Kirim Frame Serial","<suhu,pH,TDS,turbidity,DO,status>","Arduino → ESP8266") 14
Draw-Box $g 450 630 270 90 "#e0f2fe" "#0284c7" @("Publish MQTT Sensor","aquaponics/sensors/<id>","ESP8266") 15
Draw-Box $g 450 775 270 92 "#fef3c7" "#d97706" @("Node-RED Validasi","cek sensor aktif, nilai numerik, satuan","Prepare live + history + automation") 14
Draw-Box $g 110 775 270 92 "#fef3c7" "#d97706" @("Simpan Firebase","/sensors dan /history/sensors","histori tiap 2 menit") 15
Draw-Box $g 110 925 270 88 "#ecfeff" "#0891b2" @("Dashboard AquaMonitor","nilai real-time, grafik, history") 15
Draw-Diamond $g 110 1060 270 140 "#fce7f3" "#be185d" @("Monitoring","berlanjut?")
Draw-Arrow $g 245 433 245 485 "#0284c7"; Draw-Arrow $g 245 577 245 630 "#0284c7"; Draw-Arrow $g 380 675 450 675 "#0284c7"
Draw-Arrow $g 585 720 585 775 "#0284c7"; Draw-Arrow $g 450 821 380 821 "#0284c7"; Draw-Arrow $g 245 867 245 925 "#0284c7"; Draw-Arrow $g 245 1013 245 1060 "#0284c7"
Draw-PolylineArrow $g @(@(110,1130),@(75,1130),@(75,389),@(110,389)) "#0284c7"
Draw-Text $g "YA, ulangi baca sensor" 85 1110 13 $false "#475569"

Draw-Group $g 855 285 795 965 "C. Alur Otomasi dan Kontrol Aktuator"
Draw-Box $g 930 345 300 90 "#ecfeff" "#0891b2" @("User Mengatur Aktuator","mode auto, sensor, threshold, hysteresis","Dashboard AquaMonitor") 15
Draw-Box $g 930 485 300 88 "#fef3c7" "#d97706" @("Konfigurasi Tersimpan","Firebase /actuators/{id}") 15
Draw-Box $g 930 630 300 90 "#fef3c7" "#d97706" @("Node-RED Membaca Rule","enabled, mode, sensor, threshold","cache actuator rules") 15
Draw-Diamond $g 930 770 300 160 "#fce7f3" "#be185d" @("Nilai sensor","memenuhi threshold?")
Draw-Box $g 1330 805 270 90 "#fef3c7" "#d97706" @("Kirim Command MQTT","aquaponics/actuators/<id>/command","source: auto-threshold / web") 13
Draw-Box $g 1330 955 270 90 "#e0f2fe" "#0284c7" @("ESP8266 Terima Command","device + action","diteruskan ke Arduino") 15
Draw-Box $g 930 955 300 90 "#fee2e2" "#dc2626" @("Arduino Safety Control","pending action, hasActiveRelay()","pulse/cycle, lockout, no ON permanen") 14
Draw-Box $g 930 1095 300 100 "#ede9fe" "#7c3aed" @("Aktuator Bekerja Aman","pump 2s/20s, aerator 5s/20s","heater 5s, buzzer 1s","dosing 1s + lockout 30s") 14
Draw-Box $g 1330 1095 270 100 "#f1f5f9" "#475569" @("Status Aktual Relay","Arduino → ESP → MQTT","Node-RED → Firebase","dashboard: running / idle") 14
Draw-Arrow $g 1080 435 1080 485 "#d97706"; Draw-Arrow $g 1080 573 1080 630 "#d97706"; Draw-Arrow $g 1080 720 1080 770 "#d97706"
Draw-Arrow $g 1230 850 1330 850 "#d97706"; Draw-Text $g "YA" 1260 825 13 $false "#475569"
Draw-Arrow $g 1465 895 1465 955 "#7c3aed"; Draw-Arrow $g 1330 1000 1230 1000 "#7c3aed"; Draw-Arrow $g 1080 1045 1080 1095 "#7c3aed"; Draw-Arrow $g 1230 1145 1330 1145 "#7c3aed"
Draw-PolylineArrow $g @(@(930,850),@(865,850),@(865,675),@(930,675)) "#d97706"
Draw-Text $g "TIDAK, tunggu data berikutnya" 875 825 13 $false "#475569"
Draw-PolylineArrow $g @(@(1465,1195),@(1465,1290),@(245,1290),@(245,1200)) "#0f172a"

Draw-Box $g 180 1305 1440 95 "#f8fafc" "#cbd5e1" @("Keterangan: Threshold user disimpan di Firebase dan diproses Node-RED.","Arduino mengeksekusi command relay secara aman menggunakan pulse/cycle dan lockout.","Data sensor dan status aktuator kembali ke Firebase untuk dashboard AquaMonitor.") 15

Save-Jpg $bmp (Join-Path $outDir "flowchart-aquamonitor-keseluruhan-terbaru-render.jpg")
$g.Dispose(); $bmp.Dispose()

# Activity Diagram
$pair = New-Bitmap 1900 1350; $bmp = $pair[0]; $g = $pair[1]
Draw-CenterText $g @("Activity Diagram Keseluruhan Sistem AquaMonitor") 0 25 1900 40 28 $true
Draw-CenterText $g @("Interaksi pengguna, dashboard, Firebase, Node-RED, MQTT/ESP8266, Arduino, sensor, dan aktuator") 0 68 1900 30 16 $false "#475569"

$laneXs = @(40,280,540,800,1080,1360,1610); $laneWs = @(240,260,260,280,280,250,250)
$laneTitles = @("Pengguna","Dashboard","Firebase RTDB","Node-RED","MQTT & ESP8266","Arduino Uno","Sensor & Aktuator")
for($i=0;$i -lt $laneXs.Count;$i++){
  $b=Brush "#f8fafc"; $p=Pen "#cbd5e1" 1.5
  $g.FillRectangle($b,$laneXs[$i],125,$laneWs[$i],1120); $g.DrawRectangle($p,$laneXs[$i],125,$laneWs[$i],1120)
  Draw-CenterText $g @($laneTitles[$i]) $laneXs[$i] 132 $laneWs[$i] 35 14 $true
  $b.Dispose(); $p.Dispose()
}

$b=Brush "#dcfce7"; $p=Pen "#16a34a" 2.4
$g.FillEllipse($b,1457,190,56,56); $g.DrawEllipse($p,1457,190,56,56); Draw-CenterText $g @("Start") 1457 190 56 56 13 $true
$b.Dispose(); $p.Dispose()

Draw-Box $g 1385 275 200 72 "#e0f2fe" "#0284c7" @("Inisialisasi","sensor, relay, serial") 14
Draw-Box $g 1635 385 200 82 "#ede9fe" "#7c3aed" @("Sensor membaca","suhu, pH, TDS, DO,","turbidity") 14
Draw-Box $g 1385 385 200 82 "#e0f2fe" "#0284c7" @("Arduino memproses","filter pH/TDS dan","membentuk frame") 14
Draw-Box $g 1110 385 220 82 "#e0f2fe" "#0284c7" @("ESP publish sensor","ke MQTT topic","aquaponics/sensors/+") 14
Draw-Box $g 830 385 220 82 "#fef3c7" "#d97706" @("Node-RED validasi","live data, histori,","nilai untuk otomasi") 14
Draw-Box $g 570 385 200 82 "#fef3c7" "#d97706" @("Simpan data","/sensors","/history/sensors") 14
Draw-Box $g 310 385 200 82 "#ecfeff" "#0891b2" @("Tampilkan data","real-time, grafik,","status sensor") 14
Draw-Box $g 60 525 200 82 "#ecfeff" "#0891b2" @("User memantau","kondisi air dan","status sistem") 14
Draw-Diamond $g 300 525 220 120 "#fce7f3" "#be185d" @("Perlu ubah","threshold?")
Draw-Box $g 60 695 200 82 "#ecfeff" "#0891b2" @("User mengatur","mode auto, sensor,","threshold, hysteresis") 14
Draw-Box $g 310 695 200 82 "#ecfeff" "#0891b2" @("Dashboard simpan","konfigurasi aktuator","ke Firebase") 14
Draw-Box $g 570 695 200 82 "#fef3c7" "#d97706" @("RTDB update","/actuators/{id}","automation rule") 14
Draw-Box $g 830 695 220 82 "#fef3c7" "#d97706" @("Node-RED cache","enabled, mode, sensor,","threshold, hysteresis") 14
Draw-Diamond $g 820 830 240 140 "#fce7f3" "#be185d" @("Nilai sensor","melewati threshold?")
Draw-Box $g 1110 860 220 82 "#e0f2fe" "#0284c7" @("Command MQTT","device + action","ke ESP8266") 14
Draw-Box $g 1385 860 200 82 "#fee2e2" "#dc2626" @("Arduino terima","pending action +","cek relay aktif") 14
Draw-Box $g 1635 860 200 82 "#ede9fe" "#7c3aed" @("Aktuator bekerja","pulse/cycle aman","sesuai firmware") 14
Draw-Box $g 1385 1010 200 90 "#e0f2fe" "#0284c7" @("Kirim status relay","@waterPump,aerator,","heater,pump,buzzer") 14
Draw-Box $g 1110 1010 220 90 "#e0f2fe" "#0284c7" @("ESP publish status","aquaponics/actuators/","{device}/state") 14
Draw-Box $g 830 1010 220 90 "#fef3c7" "#d97706" @("Node-RED simpan","status aktual dan","history actuators") 14
Draw-Box $g 570 1010 200 90 "#fef3c7" "#d97706" @("Firebase update","/actuators","/history/actuators") 14
Draw-Box $g 310 1010 200 90 "#ecfeff" "#0891b2" @("Dashboard update","running / idle,","last active") 14

$b=Brush "#16a34a"; $p=Pen "#16a34a" 2.4
$g.FillEllipse($b,391,1166,38,38); $b.Dispose(); $p.Dispose()
Draw-CenterText $g @("Siklus monitoring berulang selama sistem aktif") 0 1218 1900 25 14 $false "#334155"

Draw-Arrow $g 1485 246 1485 275
Draw-Arrow $g 1585 426 1635 426 "#0284c7"; Draw-Arrow $g 1385 426 1330 426 "#0284c7"; Draw-Arrow $g 1110 426 1050 426 "#0284c7"; Draw-Arrow $g 830 426 770 426 "#0284c7"; Draw-Arrow $g 570 426 510 426 "#0284c7"
Draw-Arrow $g 410 467 410 525 "#0284c7"; Draw-Arrow $g 300 585 260 566
Draw-PolylineArrow $g @(@(410,645),@(410,665),@(160,665),@(160,695)) "#7c3aed"; Draw-Text $g "YA" 435 655 12 $false "#475569"
Draw-Arrow $g 260 736 310 736 "#7c3aed"; Draw-Arrow $g 510 736 570 736 "#7c3aed"; Draw-Arrow $g 770 736 830 736 "#7c3aed"; Draw-Arrow $g 940 777 940 830 "#7c3aed"
Draw-Arrow $g 1060 900 1110 900 "#7c3aed"; Draw-Text $g "YA" 1075 880 12 $false "#475569"; Draw-Arrow $g 1330 901 1385 901 "#7c3aed"; Draw-Arrow $g 1585 901 1635 901 "#7c3aed"
Draw-PolylineArrow $g @(@(1735,942),@(1735,1055),@(1585,1055)) "#0f172a"
Draw-Arrow $g 1385 1055 1330 1055 "#0284c7"; Draw-Arrow $g 1110 1055 1050 1055 "#0284c7"; Draw-Arrow $g 830 1055 770 1055 "#0284c7"; Draw-Arrow $g 570 1055 510 1055 "#0284c7"; Draw-Arrow $g 410 1100 410 1166
Draw-PolylineArrow $g @(@(820,900),@(790,900),@(790,500),@(410,500),@(410,525)) "#0f172a"; Draw-Text $g "TIDAK" 790 880 12 $false "#475569"
Draw-Box $g 110 1260 1680 60 "#ffffff" "#94a3b8" @("Catatan: threshold user diproses Node-RED, command dikirim via MQTT, dan Arduino menjalankan aktuator dengan pengaman pulse/cycle.") 15

Save-Jpg $bmp (Join-Path $outDir "activity-diagram-aquamonitor-keseluruhan-terbaru-render.jpg")
$g.Dispose(); $bmp.Dispose()

Write-Output "Rendered JPG diagrams successfully."
