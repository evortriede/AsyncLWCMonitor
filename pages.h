/*
 * Server Index Page
 */
 
static const char serverIndex[] PROGMEM =R"(
<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
   <input type='file' name='update'>
        <input type='submit' value='Update'>
</form>
<div id='prg'>progress: 0%</div>
<br>      <a href='/' style="font-size:40px">Home</a><br>
<script>
  $('form').submit(function(e)
  {
    e.preventDefault();
    var form = $('#upload_form')[0];
    var data = new FormData(form);
    $.ajax(
    {
      url: '/update',
      type: 'POST',
      data: data,
      contentType: false,
      processData:false,
      xhr: function() 
      {
        var xhr = new window.XMLHttpRequest();
        xhr.upload.addEventListener('progress', function(evt) 
        {
          if (evt.lengthComputable) 
          {
            var per = evt.loaded / evt.total;
            $('#prg').html('progress: ' + Math.round(per*100) + '%');
          }
        }, false);
        return xhr;
      },
      success:function(d, s) 
      {
        console.log('success!')
        $('#prg').html('success');
        location.replace('/');
      },
      error: function (a, b, c) 
      {
      }
     });
   });
 </script>
)";


static const char statusPage[] PROGMEM =R"(
<html>
  <head>
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen    = onOpen;
        websocket.onclose   = onClose;
        websocket.onmessage = onMessage; // <-- add this line
      }
      function onOpen(event) {
        console.log('Connection opened');
      }
      function onClose(event) {
        console.log('Connection closed');
        setTimeout(initWebSocket, 2000);
      }
      function centerText(ctx,txt,vpos)
      {
        let hpos=(200-ctx.measureText(txt).width)/2;
        ctx.strokeText(txt,hpos,vpos);
      }
      function drawMeter(name,style,metric,txt)
      {
        canvas=document.getElementById(name);
        canvas.hidden="";
        ctx=canvas.getContext("2d");
        ctx.fillStyle=style;
        ctx.clearRect(0,0,200,500);
        ctx.fillRect(25, 500-metric, 150, metric);
        ctx.font="30px Arial";
        centerText(ctx,name,30);
        centerText(ctx,txt, Math.max(60,500-(metric/2)));
      }
      function onMessage(event) 
      {
        rg=event.data.split(',');
        document.getElementById('loramsg').value=rg[0];
        document.getElementById('gallons').value=rg[1];
        document.getElementById('gph').value=rg[2];
        document.getElementById('gpd').value=rg[3];
        document.getElementById('duty').value=rg[4];
        document.getElementById('pump').value=rg[11];

        rgg=rg[1].split(' ');
        let gal=rgg[0];
        let x=gal/47; // 23688 / 500 ~= 47.4
        let pct = (gal/23688)*100;
        var style="#00FF00";
        if (pct < 33)
        {
          style = "#FF0000";
        }
        else if (pct < 50)
        {
          style = "#FFFF00";
        }

        drawMeter("TankLevel",style,x,pct.toFixed(0) + "%");
        if (rg[6]!=0)
        {
          drawMeter("Turbidity",rg[5],rg[6],rg[7]);
        }
        else
        {
          document.getElementById("Turbidity").hidden="hidden";
        }
        if (rg[2]<0)
        {
          drawMeter("Chlorine",rg[8],rg[9],rg[10]);
        }
        else
        {
          document.getElementById("Chlorine").hidden="hidden";
        }
      }
    </script>

  </head>
  <body onLoad="initWebSocket();">
    <canvas id="TankLevel" width="200" height="500"
    style="border:1px solid #c3c3c3;">
    Your browser does not support the canvas element.
    </canvas>
    <canvas id="Turbidity" width="200" height="500"
    style="border:1px solid #c3c3c3;" hidden="hidden">
    Your browser does not support the canvas element.
    </canvas>
    <canvas id="Chlorine" width="200" height="500"
    style="border:1px solid #c3c3c3;" hidden="hidden">
    Your browser does not support the canvas element.
    </canvas>
    
    <div style='font-size:250%'>
      <input id='loramsg' value='0' size='100' width='100%' style='border:none'></input> <br> 
      Pump Setting <input id="pump" value=0, style='font-size:40px; border:none'></input><br>
      Tank Level <input id='gallons' value='12345' style='font-size:40px; border:none'></input><br>
      Usage last hour <input id='gph' value='0' style='font-size:40px; border:none'></input> <br>
      Usage last 24 hours <input id='gpd' value='0' style='font-size:40px; border:none'></input> <br>
      Duty cycle <input id='duty' value='0' style='font-size:40px; border:none'></input><br>
      <a href='/details'>Details</a>
    </div>
  </body>
</html>
)";

static const char rootFmt[] PROGMEM =R"(
<html>
  <head>
  </head>
  <body>
    <div style='font-size:250%%'>
      %s <br> 
      %i gal (%i/%i) <br>
      Usage last hour %i <br>
      Usage last 24 hours %i <br>
      On %i Off %i <br>
      Elappsed minutes: %ld <br>
      Attached SSID: %s <br>
      IP Addr: %s <br>
      Last volume: %ld Last turbidity: %ld Last chlorine: %ld <br>
      <a href='/config'>Configure</a> <br>
      <a href='/ota'>Firmware Upload</a> <br>
      <a href='/'>Back</a><br>
      <a href='/reboot'>Boot</a>
    </div>
    <canvas id="myCanvas" width="300" height="150" style="border:1px solid #d3d3d3;">
      Your browser does not support the HTML5 canvas tag.
    </canvas>
    <script>
      data=[%s];
      var c = document.getElementById("myCanvas");
      c.width=window.innerWidth-20;
      var ctx = c.getContext("2d");
      ctx.beginPath();
      for (let i=0;i<data.length;i++)
      {
        ctx.lineTo(i*((window.innerWidth-20)/(data.length-1)),150-(data[i]/22.6));
      }
      ctx.stroke();
    </script>
  </body>
</html>
)";

static const char configFmt[] PROGMEM =R"(
<html>
  <head>
  </head>
  <body>
    <div style='font-size:250%%'>
      <form action="/set" method="GET">
        SSID to join <input type="text" name="ssid" style="font-size:40px" value="%s"></input><br>
        Password for SSID to join <input type="text" name="pass" style="font-size:40px" value="%s"></input><br>
        SSID for Captive Net <input type="text" name="captive_ssid" style="font-size:40px" value="%s"></input><br>
        Password for Captive Net SSID <input type="text" name="captive_pass" style="font-size:40px" value="%s"></input><br>
        Spreading Factor (7-12) <input type="text" name="sf" style="font-size:40px" value="%i"></input><br>
        Duck DNS Name <input type="text" name="dns_name" style="font-size:40px" value="%s"></input><br>
        Remote Server <input type="text" name="rmtserver" style="font-size:40px" value="%s"></input><br>
        Conversion factor <input type="number" step="0.1" name="factor" style="font-size:30px" value="%0.2f"></input><br>
        <input type="submit" style="font-size:40px"></input>
      </form>
      <br>
      <a href='/details' style="font-size:40px">Back</a><br>
      <a href='/' style="font-size:40px">Home</a><br>
    </div>
  </body>
</html>
)";
