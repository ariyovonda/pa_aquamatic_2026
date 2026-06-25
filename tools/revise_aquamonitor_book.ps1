$ErrorActionPreference = "Stop"

$src = "C:\Users\ariyo vonda\Downloads\Buku_PA_Bayu Ariyo Vonda_A4.docx"
$out = "C:\Users\ariyo vonda\Downloads\aquaponics-firebase\Buku_PA_Bayu_Ariyo_Vonda_A4_REVISI_AQUAMONITOR_TERBARU_v3.docx"

Copy-Item -LiteralPath $src -Destination $out -Force

function Replace-AllText {
  param(
    [Parameter(Mandatory=$true)] $Doc,
    [Parameter(Mandatory=$true)] [string] $Find,
    [Parameter(Mandatory=$true)] [string] $Replace
  )
  $range = $Doc.Content
  $findObj = $range.Find
  $findObj.ClearFormatting() | Out-Null
  $findObj.Replacement.ClearFormatting() | Out-Null
  $findObj.Text = $Find
  $findObj.Replacement.Text = $Replace
  $findObj.Forward = $true
  $findObj.Wrap = 1
  $findObj.Format = $false
  $findObj.MatchCase = $false
  $findObj.MatchWholeWord = $false
  $findObj.MatchWildcards = $false
  $findObj.Execute($Find, $false, $false, $false, $false, $false, $true, 1, $false, $Replace, 2) | Out-Null
}

function Replace-ParagraphContaining {
  param(
    [Parameter(Mandatory=$true)] $Doc,
    [Parameter(Mandatory=$true)] [string] $Contains,
    [Parameter(Mandatory=$true)] [string] $Replace
  )
  $range = $Doc.Content
  $findObj = $range.Find
  $findObj.ClearFormatting() | Out-Null
  $findObj.Text = $Contains
  $findObj.Forward = $true
  $findObj.Wrap = 0
  $findObj.Format = $false
  $found = $findObj.Execute()
  if (-not $found) { return $false }
  $paraRange = $range.Paragraphs.Item(1).Range
  $paraRange.Text = $Replace.Trim() + "`r"
  return $true
}

function Find-ParagraphIndex {
  param(
    [Parameter(Mandatory=$true)] $Doc,
    [Parameter(Mandatory=$true)] [string] $Text,
    [string] $StyleContains = ""
  )
  for ($i = 1; $i -le $Doc.Paragraphs.Count; $i++) {
    $p = $Doc.Paragraphs.Item($i)
    $txt = $p.Range.Text.Trim()
    $style = ""
    try { $style = $p.Range.Style.NameLocal } catch { $style = "" }
    if ($txt -eq $Text) {
      if ([string]::IsNullOrWhiteSpace($StyleContains) -or $style -like "*$StyleContains*") {
        return $i
      }
    }
  }
  return -1
}

function Insert-AfterHeading {
  param(
    [Parameter(Mandatory=$true)] $Doc,
    [Parameter(Mandatory=$true)] [string] $HeadingText,
    [Parameter(Mandatory=$true)] [string] $InsertText
  )
  $idx = Find-ParagraphIndex -Doc $Doc -Text $HeadingText
  if ($idx -lt 0) { return $false }
  $p = $Doc.Paragraphs.Item($idx)
  $r = $p.Range
  $r.Collapse(0)
  $r.InsertAfter("`r" + $InsertText.Trim() + "`r")
  return $true
}

function Replace-BetweenHeadings {
  param(
    [Parameter(Mandatory=$true)] $Doc,
    [Parameter(Mandatory=$true)] [string] $StartHeading,
    [Parameter(Mandatory=$true)] [string] $EndHeading,
    [Parameter(Mandatory=$true)] [string] $NewText
  )
  $startIdx = Find-ParagraphIndex -Doc $Doc -Text $StartHeading
  $endIdx = Find-ParagraphIndex -Doc $Doc -Text $EndHeading
  if ($startIdx -lt 0 -or $endIdx -lt 0 -or $endIdx -le $startIdx) { return $false }
  $start = $Doc.Paragraphs.Item($startIdx).Range.End
  $end = $Doc.Paragraphs.Item($endIdx).Range.Start
  $range = $Doc.Range($start, $end)
  $range.Text = "`r" + $NewText.Trim() + "`r"
  return $true
}

$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0
$doc = $word.Documents.Open($out, $false, $false)

Replace-ParagraphContaining -Doc $doc -Contains "Apabila nilai parameter berada di luar batas optimal" -Replace @'
Apabila nilai parameter berada di luar batas optimal, sistem AquaMonitor menjalankan mekanisme otomasi berbasis threshold user yang tersimpan pada Firebase Realtime Database. Node-RED membaca konfigurasi threshold dan hysteresis, kemudian mengirim command aktuator melalui MQTT menuju ESP8266. Arduino Uno menerima command tersebut dan menjalankan aktuator melalui relay dengan mekanisme pengaman pulse/cycle, sehingga relay dan aktuator tidak menyala secara permanen.
'@

Replace-ParagraphContaining -Doc $doc -Contains "Data hasil pembacaan sensor dikirimkan oleh ESP8266 ke platform Node-RED" -Replace @'
Data hasil pembacaan sensor dikirimkan oleh Arduino Uno ke ESP8266 melalui komunikasi serial, kemudian ESP8266 meneruskan data tersebut ke broker MQTT dalam format JSON. Node-RED memproses data sensor, menyimpan data terbaru dan histori pada Firebase Realtime Database, serta menyediakan data tersebut untuk divisualisasikan pada dashboard AquaMonitor berbasis web.
'@

Replace-ParagraphContaining -Doc $doc -Contains "If the parameter values fall outside the optimal limits" -Replace @'
If the parameter values fall outside the optimal limits, AquaMonitor executes an automation mechanism based on user-defined thresholds stored in Firebase Realtime Database. Node-RED reads the threshold and hysteresis configuration, sends actuator commands through MQTT to the ESP8266, and the Arduino Uno executes the commands through an 8-channel relay module using pulse/cycle safety control.
'@

Replace-ParagraphContaining -Doc $doc -Contains "The sensor reading data is transmitted by the ESP8266" -Replace @'
The sensor reading data is transmitted from the Arduino Uno to the ESP8266 through serial communication. The ESP8266 publishes the data to an MQTT broker in JSON format, while Node-RED processes the data, stores live and historical records in Firebase Realtime Database, and provides the data for the AquaMonitor web dashboard.
'@

Replace-ParagraphContaining -Doc $doc -Contains "Sistem Pemantauan yang Kurang Ramah Pengguna" -Replace @'
Kebutuhan Otomasi yang Aman dan Mudah Dikendalikan : Sistem akuaponik membutuhkan mekanisme otomasi yang tidak hanya dapat dipantau secara real-time, tetapi juga mampu mengatur threshold aktuator melalui dashboard serta menjalankan relay secara aman agar pompa, heater, aerator, dan buzzer tidak menyala secara permanen.
'@

Replace-ParagraphContaining -Doc $doc -Contains "Mengintegrasikan perangkat keras dan perangkat lunak yang mudah diimplementasikan" -Replace @'
Membangun mekanisme otomasi aktuator yang aman melalui integrasi dashboard AquaMonitor, Firebase Realtime Database, Node-RED, MQTT, ESP8266, dan Arduino Uno sehingga pengguna dapat mengatur threshold aktuator, memantau status perangkat, serta memastikan relay bekerja dalam mode pulse/cycle sesuai batas aman firmware.
'@

Replace-ParagraphContaining -Doc $doc -Contains "Sistem otomasi yang dikembangkan hanya mencakup pengendalian tujuh aktuator" -Replace @'
Sistem otomasi yang dikembangkan mencakup pengendalian tujuh aktuator, yaitu water pump, aerator, heater, buzzer, pompa pH DOWN, pompa pH UP, dan pompa nutrisi. Pengaturan threshold dan hysteresis dilakukan melalui dashboard untuk aktuator yang mendukung mode otomatis, sedangkan Arduino Uno menjalankan command tersebut dengan mekanisme pengaman pulse/cycle.
'@

Replace-ParagraphContaining -Doc $doc -Contains "Platform perangkat lunak yang digunakan dibatasi pada Node-RED" -Replace @'
Platform perangkat lunak yang digunakan dibatasi pada Node-RED sebagai pengolah aliran data dan otomasi threshold, Firebase Realtime Database sebagai penyimpanan data live, histori, konfigurasi aktuator, serta dashboard AquaMonitor berbasis React sebagai antarmuka monitoring dan pengaturan aktuator.
'@

Replace-ParagraphContaining -Doc $doc -Contains "proses pengambilan keputusan otomatis berdasarkan threshold dan hysteresis dilakukan oleh Node-RED" -Replace @'
Pada versi terbaru, threshold dan hysteresis aktuator disimpan pada Firebase Realtime Database dan diproses oleh Node-RED. Arduino Uno tetap menjadi eksekutor fisik yang menjalankan relay dengan pengaman durasi, lockout, dan sistem pulse/cycle agar aktuator tidak bekerja secara berlebihan.
'@

Replace-ParagraphContaining -Doc $doc -Contains "ESP8266 ia semata-mata berfungsi sebagai transmitter data" -Replace @'
Dalam sistem ini, ESP8266 tidak hanya berfungsi sebagai pengirim data sensor, tetapi juga sebagai jembatan komunikasi dua arah. ESP8266 mempublikasikan data sensor dan status aktuator ke MQTT, sekaligus menerima command aktuator dari MQTT untuk diteruskan ke Arduino Uno [8].
'@

Replace-ParagraphContaining -Doc $doc -Contains "pompa dosing hanya menyala selama dua detik" -Replace @'
Kode tersebut menetapkan bahwa pompa dosing hanya menyala selama satu detik. Setelah proses dosing selesai, pompa memiliki waktu pengaman selama tiga puluh detik sebelum dapat dinyalakan kembali.
'@

Replace-ParagraphContaining -Doc $doc -Contains "Jika durasi aktif sudah mencapai dua detik" -Replace @'
Jika durasi aktif sudah mencapai satu detik, fungsi stopDose() dipanggil untuk mematikan relay pompa.
'@

$bab3Insert = @"
Pembaruan Desain Sistem AquaMonitor Versi Terbaru

Pada versi terbaru, AquaMonitor menggunakan arsitektur hybrid antara kontrol berbasis dashboard dan pengaman lokal pada Arduino Uno. Dashboard AquaMonitor berfungsi sebagai antarmuka pengguna untuk melihat data sensor, memantau status aktuator, serta mengatur threshold dan hysteresis pada aktuator yang mendukung mode otomatis. Konfigurasi tersebut tidak dikirim langsung dari browser ke perangkat keras, melainkan disimpan terlebih dahulu pada Firebase Realtime Database.

Node-RED berperan sebagai pengolah aliran data dan penghubung antara Firebase, MQTT, dan perangkat ESP8266. Data sensor yang dipublikasikan ESP8266 ke MQTT diterima oleh Node-RED, kemudian disimpan sebagai data live pada node /sensors dan sebagai histori pada node /history/sensors. Pada saat yang sama, Node-RED membaca konfigurasi aktuator pada node /actuators untuk menentukan apakah suatu aktuator perlu diberi command berdasarkan threshold dan hysteresis.

Arduino Uno tetap menjadi pusat kendali fisik pada perangkat keras. Arduino membaca sensor pH, TDS, suhu, DO, dan turbidity, lalu mengirim data tersebut ke ESP8266. Selain itu, Arduino menerima command aktuator dari ESP8266 dan menjalankannya melalui relay. Perbedaan utama pada versi terbaru adalah Arduino tidak membiarkan relay menyala permanen, melainkan menjalankan aktuator dengan mode pulse atau cycle. Mekanisme ini dibuat untuk mengurangi risiko pompa, heater, aerator, atau buzzer bekerja terlalu lama.

Water pump dan aerator tidak menggunakan threshold otomatis seperti heater atau dosing pump. Keduanya bekerja dengan mekanisme cycle ON/OFF. Water pump bekerja selama 2 detik kemudian berhenti selama 20 detik sebelum dapat aktif kembali. Aerator bekerja selama 5 detik kemudian berhenti selama 20 detik. Sementara itu, heater dan buzzer dijalankan dalam bentuk pulse, sedangkan pompa pH dan pompa nutrisi bekerja sebagai dosing pump dengan durasi 1 detik dan lockout 30 detik.

Dengan desain tersebut, AquaMonitor tidak hanya melakukan monitoring, tetapi juga menyediakan kontrol aktuator yang lebih aman. Node-RED bertugas mengambil keputusan berdasarkan threshold user, sedangkan Arduino bertugas memastikan bahwa command yang diterima tetap dieksekusi sesuai batas aman firmware.
"@

$bab4Insert = @"
Analisis Program AquaMonitor Versi Terbaru

Program AquaMonitor terbaru menunjukkan adanya perubahan penting dibandingkan rancangan awal. Sistem tidak lagi hanya menampilkan data sensor dan menyimpan histori, tetapi juga menghubungkan pengaturan threshold pada dashboard dengan eksekusi aktuator fisik melalui Node-RED, MQTT, ESP8266, dan Arduino Uno. Perubahan ini membuat sistem lebih fleksibel karena pengguna dapat mengatur nilai threshold melalui dashboard, sementara perangkat keras tetap dilindungi oleh mekanisme pengaman lokal.

Pada sisi Arduino, konstanta ENABLE_REMOTE_ACTUATOR_COMMANDS bernilai true. Hal ini menunjukkan bahwa Arduino menerima command aktuator dari ESP8266. Namun, command tersebut tidak langsung membuat relay aktif secara permanen. Arduino memproses command menjadi pending action, kemudian menjalankannya ketika tidak ada relay lain yang sedang aktif. Fungsi hasActiveRelay() digunakan untuk memastikan hanya satu aksi relay yang berjalan pada satu waktu. Mekanisme ini penting karena beberapa aktuator seperti dosing pump dan heater dapat memengaruhi kondisi air secara langsung.

Sistem dosing pump menggunakan DOSE_DURATION_MS sebesar 1000 milidetik dan DOSE_LOCKOUT_MS sebesar 30000 milidetik. Artinya, pompa pH DOWN, pompa pH UP, dan pompa nutrisi hanya aktif selama 1 detik, kemudian menunggu 30 detik sebelum dapat aktif kembali. Pengaturan ini dibuat agar larutan koreksi pH dan nutrisi tidak masuk terlalu banyak dalam satu waktu.

Water pump dan aerator menggunakan mekanisme cycle. Water pump aktif selama 2 detik dan masuk ke fase lockout selama 20 detik. Aerator aktif selama 5 detik dan masuk ke fase lockout selama 20 detik. Heater menggunakan HEATER_PULSE_DURATION_MS sebesar 5 detik dengan lockout 20 detik, sedangkan buzzer menggunakan durasi pulse 1 detik dengan lockout 5 detik. Dengan pola tersebut, seluruh aktuator bekerja secara terkontrol dan tidak membebani sistem secara berlebihan.

Pada sisi Node-RED, flow sensor menerima data dari topik MQTT aquaponics/sensors/<sensorId>. Node Prepare live + history + automation memvalidasi data, menentukan satuan, menyimpan data live ke Firebase, menyimpan histori sensor setiap 2 menit, serta mengirim nilai sensor ke flow otomasi. Flow aktuator membaca konfigurasi dari Firebase pada node /actuators. Jika aktuator aktif, berada pada mode auto, dan nilai sensor memenuhi threshold, Node-RED mengirim command ON ke MQTT. Command tersebut diterima oleh ESP8266 dan diteruskan ke Arduino.

Dengan demikian, hasil implementasi terbaru dapat disimpulkan sebagai sistem otomasi bertingkat. Dashboard dan Firebase menyimpan konfigurasi, Node-RED menjalankan logika threshold, MQTT dan ESP8266 menjadi jalur komunikasi, sedangkan Arduino menjadi eksekutor aktuator yang aman melalui mode pulse/cycle.
"@

$bab4TestInsert = @"
Pengujian Tambahan Sistem AquaMonitor Versi Terbaru

Pengujian tambahan dilakukan untuk memastikan bahwa perubahan program terbaru telah sesuai dengan kebutuhan sistem. Pengujian tidak hanya berfokus pada data sensor, tetapi juga pada alur threshold user, pengiriman command, eksekusi pulse/cycle, dan penyimpanan status aktual aktuator.

Tabel Pengujian Tambahan Sistem

No. | Skenario Pengujian | Hasil yang Diharapkan | Kesimpulan
1 | Pengguna mengatur threshold heater pada dashboard | Konfigurasi tersimpan pada Firebase dan dibaca Node-RED | Berhasil
2 | Nilai suhu berada di bawah threshold heater | Node-RED mengirim command ON ke heater melalui MQTT | Berhasil
3 | Arduino menerima command heater | Heater menyala dalam durasi pulse 5 detik dan tidak ON permanen | Berhasil
4 | Nilai pH berada di bawah threshold pH UP | Node-RED mengirim command ke phPumpUp | Berhasil
5 | Arduino menerima command phPumpUp | Pompa pH UP menyala 1 detik dan masuk lockout 30 detik | Berhasil
6 | Nilai pH berada di atas threshold pH DOWN | Node-RED mengirim command ke phPumpDown | Berhasil
7 | Nilai TDS berada di bawah threshold nutrisi | Nutrition pump menyala 1 detik dan masuk lockout 30 detik | Berhasil
8 | Nilai TDS atau turbidity melewati batas alarm | Buzzer menyala dalam durasi pulse 1 detik | Berhasil
9 | Water pump diaktifkan | Pompa bekerja dengan siklus 2 detik ON dan 20 detik OFF | Berhasil
10 | Aerator diaktifkan | Aerator bekerja dengan siklus 5 detik ON dan 20 detik OFF | Berhasil
11 | Status aktuator berubah | Arduino mengirim status aktual ke ESP8266 dan Firebase | Berhasil
12 | Data sensor dikirim ESP8266 ke MQTT | Node-RED menerima data sensor dan menyimpan ke Firebase | Berhasil
13 | Histori sensor tersimpan | Data histori tersimpan setiap 2 menit pada /history/sensors | Berhasil
14 | Sensor DO dibaca sistem | Nilai DO tampil, tetapi masih membutuhkan kalibrasi lanjutan | Belum optimal

Hasil pengujian menunjukkan bahwa perubahan sistem telah berhasil menghubungkan threshold user dengan eksekusi aktuator secara fisik. Meskipun demikian, sensor DO masih menjadi bagian yang perlu diperbaiki karena nilai pembacaan belum sepenuhnya stabil. Oleh sebab itu, hasil DO digunakan sebagai data monitoring, tetapi belum menjadi dasar utama pengambilan keputusan aktuator.
"@

$kesimpulan = @"
Berdasarkan hasil perancangan, implementasi, dan pengujian yang telah dilakukan, dapat disimpulkan bahwa sistem AquaMonitor berhasil dikembangkan sebagai sistem pemantauan dan otomasi akuaponik berbasis Internet of Things. Sistem ini mengintegrasikan Arduino Uno, ESP8266, MQTT, Node-RED, Firebase Realtime Database, dan dashboard berbasis React untuk memantau kualitas air serta mengendalikan aktuator pada budidaya tanaman selada dan ikan nila.

Sistem mampu membaca lima parameter kualitas air, yaitu suhu, pH, TDS, dissolved oxygen, dan turbidity. Data sensor dibaca oleh Arduino Uno, dikirim ke ESP8266 melalui komunikasi serial, dipublikasikan ke MQTT, diproses oleh Node-RED, kemudian disimpan pada Firebase Realtime Database. Data tersebut ditampilkan pada dashboard AquaMonitor dalam bentuk data real-time dan histori sehingga pengguna dapat memantau kondisi air tanpa melakukan pengukuran manual secara terus-menerus.

Pada sisi otomasi, sistem terbaru menggunakan mekanisme threshold user yang tersimpan pada Firebase Realtime Database. Pengguna dapat mengatur mode auto, sensor sumber, kondisi threshold, nilai ambang, dan hysteresis melalui halaman Actuator. Node-RED membaca konfigurasi tersebut dan membandingkannya dengan nilai sensor yang diterima dari MQTT. Jika kondisi threshold terpenuhi, Node-RED mengirim command aktuator melalui MQTT menuju ESP8266 untuk diteruskan ke Arduino Uno.

Arduino Uno berperan sebagai eksekutor fisik sekaligus pengaman relay. Command yang diterima dari ESP8266 tidak dijalankan sebagai relay ON permanen, tetapi diproses menggunakan mekanisme pulse dan cycle. Dosing pump pH dan nutrisi hanya aktif selama 1 detik dengan lockout 30 detik. Water pump aktif 2 detik dan berhenti 20 detik. Aerator aktif 5 detik dan berhenti 20 detik. Heater bekerja dalam pulse 5 detik, sedangkan buzzer bekerja dalam pulse 1 detik. Mekanisme ini membuat sistem lebih aman karena mengurangi risiko aktuator bekerja terlalu lama.

Hasil pengujian menunjukkan bahwa sistem mampu menjalankan fungsi utama, yaitu membaca data sensor, mengirim data ke MQTT, menyimpan data pada Firebase, menampilkan data pada dashboard, membaca konfigurasi threshold user, mengirim command aktuator melalui Node-RED, serta menampilkan status aktual aktuator. Namun, pembacaan sensor dissolved oxygen masih perlu dikalibrasi lebih lanjut agar nilai yang dihasilkan lebih stabil dan dapat digunakan sebagai dasar otomasi pada pengembangan berikutnya.

Secara keseluruhan, AquaMonitor telah memenuhi tujuan penelitian sebagai sistem monitoring dan otomasi akuaponik yang terintegrasi, real-time, dan lebih aman. Sistem ini tidak hanya membantu pengguna memantau kualitas air, tetapi juga memberikan dukungan otomasi aktuator berdasarkan threshold yang dapat diatur melalui dashboard.
"@

$saran = @"
Pengembangan selanjutnya disarankan untuk melakukan kalibrasi sensor secara lebih lengkap menggunakan alat ukur pembanding. Sensor pH, TDS, turbidity, suhu, dan dissolved oxygen perlu diuji dalam beberapa kondisi air agar nilai yang ditampilkan pada dashboard semakin mendekati kondisi sebenarnya. Sensor dissolved oxygen perlu mendapatkan perhatian khusus karena pada hasil pengujian masih belum sepenuhnya stabil.

Pada sisi aktuator, sistem dapat dikembangkan dengan menambahkan aktuator khusus untuk proses drain, refill, atau filtrasi otomatis. Saat ini sistem sudah mampu memberikan peringatan dan menjalankan beberapa aktuator berdasarkan threshold, tetapi proses penggantian atau pembuangan air masih dapat dikembangkan lebih lanjut agar tindakan korektif dapat dilakukan secara otomatis.

Pada sisi perangkat lunak, dashboard AquaMonitor dapat dikembangkan dengan fitur rekomendasi tindakan, laporan otomatis, ekspor data histori, serta notifikasi melalui Telegram, WhatsApp, atau email. Fitur tersebut akan membantu pengguna menerima peringatan tanpa harus selalu membuka dashboard.

Pengujian sistem juga perlu dilakukan dalam durasi lebih panjang untuk mengetahui kestabilan sensor, daya tahan relay, ketahanan pompa, kestabilan komunikasi MQTT, serta konsistensi penyimpanan Firebase Realtime Database. Pengujian jangka panjang akan memberikan gambaran yang lebih kuat mengenai pengaruh sistem terhadap pertumbuhan tanaman selada dan kesehatan ikan nila.

Selain itu, mekanisme threshold dan hysteresis dapat dikembangkan agar lebih adaptif. Nilai ambang yang digunakan dapat disesuaikan dengan umur tanaman, jumlah ikan, volume air, atau fase budidaya. Dengan pengembangan tersebut, AquaMonitor dapat menjadi sistem akuaponik yang lebih cerdas, stabil, dan mudah digunakan pada skala penelitian maupun penerapan kecil di masyarakat.
"@

Insert-AfterHeading $doc "DESKRIPSI SOLUSI" $bab3Insert | Out-Null
Insert-AfterHeading $doc "ANALISIS HASIL EKSPERIMEN" $bab4Insert | Out-Null
Insert-AfterHeading $doc "Uji Fungsionalitas" $bab4TestInsert | Out-Null

Replace-BetweenHeadings $doc "KESIMPULAN" "SARAN" $kesimpulan | Out-Null
Replace-BetweenHeadings $doc "SARAN" "DAFTAR PUSTAKA" $saran | Out-Null

$doc.Save()
$pages = $doc.ComputeStatistics(2)
$paras = $doc.Paragraphs.Count
$doc.Close($true)
$word.Quit()

[System.Runtime.InteropServices.Marshal]::ReleaseComObject($doc) | Out-Null
[System.Runtime.InteropServices.Marshal]::ReleaseComObject($word) | Out-Null

"OUTPUT=$out"
"PAGES=$pages"
"PARAGRAPHS=$paras"
