$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path (Get-Location) "docs"

function New-Canvas($w, $h) {
  $bmp = New-Object System.Drawing.Bitmap($w, $h)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
  $g.Clear([System.Drawing.Color]::White)
  return @($bmp, $g)
}

function Font($size, $style="Regular") { New-Object System.Drawing.Font("Arial", $size, [System.Drawing.FontStyle]::$style) }
function Brush($hex) { New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml($hex)) }
function Pen($hex, $w=2) { New-Object System.Drawing.Pen([System.Drawing.ColorTranslator]::FromHtml($hex), $w) }

function RoundedPath($x,$y,$w,$h,$r) {
  $path = New-Object System.Drawing.Drawing2D.GraphicsPath
  $d = $r * 2
  $path.AddArc($x,$y,$d,$d,180,90)
  $path.AddArc($x+$w-$d,$y,$d,$d,270,90)
  $path.AddArc($x+$w-$d,$y+$h-$d,$d,$d,0,90)
  $path.AddArc($x,$y+$h-$d,$d,$d,90,90)
  $path.CloseFigure()
  return $path
}

function TextCenter($g,$text,$x,$y,$w,$h,$size=18,$bold=$false,$color="#0f172a") {
  $font = Font $size $(if($bold){"Bold"}else{"Regular"})
  $brush = Brush $color
  $fmt = New-Object System.Drawing.StringFormat
  $fmt.Alignment = [System.Drawing.StringAlignment]::Center
  $fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
  $rect = New-Object System.Drawing.RectangleF($x,$y,$w,$h)
  $g.DrawString($text,$font,$brush,$rect,$fmt)
  $font.Dispose(); $brush.Dispose(); $fmt.Dispose()
}

function TextLeft($g,$text,$x,$y,$size=16,$bold=$false,$color="#0f172a") {
  $font = Font $size $(if($bold){"Bold"}else{"Regular"})
  $brush = Brush $color
  $g.DrawString($text,$font,$brush,[float]$x,[float]$y)
  $font.Dispose(); $brush.Dispose()
}

function Box($g,$x,$y,$w,$h,$fill,$stroke,$text,$size=17,$bold=$false) {
  $path = RoundedPath $x $y $w $h 22
  $b=Brush $fill; $p=Pen $stroke 2.4
  $g.FillPath($b,$path); $g.DrawPath($p,$path)
  TextCenter $g $text $x $y $w $h $size $bold
  $b.Dispose(); $p.Dispose(); $path.Dispose()
}

function Diamond($g,$x,$y,$w,$h,$fill,$stroke,$text,$size=17) {
  $pts = @(
    (New-Object System.Drawing.PointF(($x+$w/2),$y)),
    (New-Object System.Drawing.PointF(($x+$w),($y+$h/2))),
    (New-Object System.Drawing.PointF(($x+$w/2),($y+$h))),
    (New-Object System.Drawing.PointF($x,($y+$h/2)))
  )
  $b=Brush $fill; $p=Pen $stroke 2.4
  $g.FillPolygon($b,$pts); $g.DrawPolygon($p,$pts)
  TextCenter $g $text $x $y $w $h $size $false
  $b.Dispose(); $p.Dispose()
}

function Arrow($g,$x1,$y1,$x2,$y2,$color="#0f172a",$w=2.6) {
  $p=Pen $color $w
  $cap=New-Object System.Drawing.Drawing2D.AdjustableArrowCap(6,6,$true)
  $p.CustomEndCap=$cap
  $g.DrawLine($p,[float]$x1,[float]$y1,[float]$x2,[float]$y2)
  $cap.Dispose(); $p.Dispose()
}

function PolyArrow($g,$pts,$color="#0f172a") {
  $p=Pen $color 2.6
  for($i=0;$i -lt $pts.Count-2;$i++){
    $g.DrawLine($p,[float]$pts[$i][0],[float]$pts[$i][1],[float]$pts[$i+1][0],[float]$pts[$i+1][1])
  }
  $cap=New-Object System.Drawing.Drawing2D.AdjustableArrowCap(6,6,$true)
  $p.CustomEndCap=$cap
  $a=$pts[$pts.Count-2]; $b=$pts[$pts.Count-1]
  $g.DrawLine($p,[float]$a[0],[float]$a[1],[float]$b[0],[float]$b[1])
  $cap.Dispose(); $p.Dispose()
}

function SaveJpg($bmp,$path) {
  $enc=[System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object {$_.MimeType -eq "image/jpeg"}
  $params=New-Object System.Drawing.Imaging.EncoderParameters(1)
  $params.Param[0]=New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality,95L)
  $bmp.Save($path,$enc,$params)
}

# Simple Flowchart
$pair=New-Canvas 1500 1800; $bmp=$pair[0]; $g=$pair[1]
TextCenter $g "Flowchart Kerja Sistem AquaMonitor" 0 30 1500 44 30 $true
TextCenter $g "Alur sederhana monitoring air dan kendali perangkat akuaponik" 0 78 1500 28 17 $false "#475569"

$cx=750; $w=470; $h=82; $x=$cx-$w/2
$ys=@(145,270,395,520,645,770,895)
$texts=@(
  "Mulai",
  "Sensor membaca kualitas air`n(suhu, pH, TDS, DO, kekeruhan)",
  "Arduino mengolah data sensor",
  "ESP8266 mengirim data ke sistem",
  "Node-RED menerima dan memeriksa data",
  "Firebase menyimpan data sensor",
  "Dashboard menampilkan kondisi air"
)
$fills=@("#dcfce7","#e0f2fe","#e0f2fe","#e0f2fe","#fef3c7","#fef3c7","#ecfeff")
$strokes=@("#16a34a","#0284c7","#0284c7","#0284c7","#d97706","#d97706","#0891b2")
for($i=0;$i -lt $ys.Count;$i++){
  Box $g $x $ys[$i] $w $h $fills[$i] $strokes[$i] $texts[$i] 17 ($i -eq 0)
  if($i -lt $ys.Count-1){ Arrow $g $cx ($ys[$i]+$h) $cx $ys[$i+1] }
}

Diamond $g 535 1030 430 150 "#fce7f3" "#be185d" "Nilai sensor`nmelewati batas?" 18
Arrow $g $cx 977 $cx 1030

Box $g 165 1230 390 95 "#f1f5f9" "#475569" "Tidak`nSistem lanjut monitoring" 17
Box $g 945 1215 390 110 "#fef3c7" "#d97706" "Ya`nNode-RED mengirim`nperintah perangkat" 17
Arrow $g 535 1105 555 1245 "#475569"; TextLeft $g "TIDAK" 455 1165 15 $true "#475569"
Arrow $g 965 1105 945 1270 "#d97706"; TextLeft $g "YA" 985 1165 15 $true "#d97706"

Box $g 945 1385 390 110 "#fee2e2" "#dc2626" "Arduino menjalankan`nperangkat dengan pengaman" 17
Box $g 945 1550 390 95 "#ede9fe" "#7c3aed" "Status perangkat`ndiperbarui di dashboard" 17
Arrow $g 1140 1325 1140 1385 "#7c3aed"
Arrow $g 1140 1495 1140 1550 "#7c3aed"
PolyArrow $g @(@(945,1598),@(750,1598),@(750,1710),@(80,1710),@(80,186),@(515,186)) "#0f172a"
PolyArrow $g @(@(360,1230),@(360,1710),@(80,1710),@(80,186),@(515,186)) "#0f172a"

Box $g 260 1690 980 70 "#f8fafc" "#cbd5e1" "Catatan: perangkat tidak menyala terus-menerus. Arduino membatasi waktu kerja agar lebih aman." 16
SaveJpg $bmp (Join-Path $outDir "flowchart-aquamonitor-sederhana.jpg")
$g.Dispose(); $bmp.Dispose()

# Simple Activity Diagram
$pair=New-Canvas 1700 1200; $bmp=$pair[0]; $g=$pair[1]
TextCenter $g "Activity Diagram Kerja Sistem AquaMonitor" 0 30 1700 44 30 $true
TextCenter $g "Versi sederhana: pengguna, dashboard, sistem IoT, dan perangkat akuaponik" 0 78 1700 28 17 $false "#475569"

$lanes=@(
  @{x=50;w=360;t="Pengguna"},
  @{x=410;w=400;t="Dashboard AquaMonitor"},
  @{x=810;w=430;t="Sistem IoT`n(Firebase, Node-RED, MQTT)"},
  @{x=1240;w=410;t="Perangkat Akuaponik`n(Sensor, Arduino, Aktuator)"}
)
foreach($ln in $lanes){
  $b=Brush "#f8fafc"; $p=Pen "#cbd5e1" 1.5
  $g.FillRectangle($b,$ln.x,130,$ln.w,980); $g.DrawRectangle($p,$ln.x,130,$ln.w,980)
  TextCenter $g $ln.t $ln.x 145 $ln.w 55 16 $true
  $b.Dispose(); $p.Dispose()
}

$b=Brush "#dcfce7"; $p=Pen "#16a34a" 2.4
$g.FillEllipse($b,1420,225,58,58); $g.DrawEllipse($p,1420,225,58,58); TextCenter $g "Start" 1420 225 58 58 13 $true
$b.Dispose(); $p.Dispose()

Box $g 1285 330 320 78 "#ede9fe" "#7c3aed" "Sensor membaca`nkondisi air" 17
Box $g 1285 455 320 78 "#e0f2fe" "#0284c7" "Arduino mengolah`ndan mengirim data" 17
Box $g 865 455 320 78 "#fef3c7" "#d97706" "Sistem menerima`ndan menyimpan data" 17
Box $g 455 455 310 78 "#ecfeff" "#0891b2" "Dashboard menampilkan`ndata terbaru" 17
Box $g 95 455 270 78 "#ecfeff" "#0891b2" "Pengguna melihat`nkondisi air" 17

Diamond $g 485 610 250 130 "#fce7f3" "#be185d" "Perlu mengatur`nbatas sensor?" 16
Box $g 95 795 270 82 "#ecfeff" "#0891b2" "Pengguna mengatur`nbatas nilai sensor" 16
Box $g 455 795 310 82 "#ecfeff" "#0891b2" "Dashboard menyimpan`npengaturan" 16
Box $g 865 795 320 82 "#fef3c7" "#d97706" "Sistem memeriksa`nnilai sensor" 16
Diamond $g 895 940 260 130 "#fce7f3" "#be185d" "Nilai melewati`nbatas?" 16
Box $g 1285 915 320 82 "#fee2e2" "#dc2626" "Arduino menyalakan`nperangkat sebentar" 16
Box $g 1285 1030 320 64 "#ede9fe" "#7c3aed" "Status perangkat`ndikirim kembali" 16
Box $g 455 1015 310 78 "#ecfeff" "#0891b2" "Dashboard memperbarui`nstatus perangkat" 16

Arrow $g 1449 283 1449 330
Arrow $g 1445 408 1445 455 "#0284c7"
Arrow $g 1285 494 1185 494 "#0284c7"
Arrow $g 865 494 765 494 "#0284c7"
Arrow $g 455 494 365 494 "#0284c7"
Arrow $g 610 533 610 610 "#0f172a"
Arrow $g 485 675 365 836 "#7c3aed"; TextLeft $g "YA" 435 715 14 $true "#7c3aed"
Arrow $g 365 836 455 836 "#7c3aed"
Arrow $g 765 836 865 836 "#7c3aed"
Arrow $g 1025 877 1025 940 "#7c3aed"
Arrow $g 1155 1005 1285 956 "#7c3aed"; TextLeft $g "YA" 1170 952 14 $true "#7c3aed"
Arrow $g 1445 997 1445 1030 "#7c3aed"
PolyArrow $g @(@(1285,1062),@(765,1062)) "#0284c7"
Arrow $g 610 1015 610 740 "#0f172a"
PolyArrow $g @(@(735,675),@(810,675),@(810,494),@(865,494)) "#0f172a"; TextLeft $g "TIDAK" 740 650 14 $true "#475569"
PolyArrow $g @(@(895,1005),@(810,1005),@(810,494),@(865,494)) "#0f172a"; TextLeft $g "TIDAK" 820 985 14 $true "#475569"

Box $g 235 1125 1230 50 "#ffffff" "#94a3b8" "Inti sistem: data air dipantau, batas sensor diperiksa, lalu perangkat bekerja sebentar sesuai pengaman." 16

SaveJpg $bmp (Join-Path $outDir "activity-diagram-aquamonitor-sederhana.jpg")
$g.Dispose(); $bmp.Dispose()

Write-Output "Simple JPG diagrams rendered."
