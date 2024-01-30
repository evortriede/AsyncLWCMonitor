<?php
$file=fopen("data.txt","r");
$data=fgets($file);
fclose($file);
?>
<html>
<head>
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" /> 
<meta http-equiv="Pragma" content="no-cache" /> 
<meta http-equiv="Expires" content="0" />
<script>
function setDivText(msg)
{
  window.parent.postMessage(msg, "*");
}
</script>
</head>
<body onload="setDivText('<?php echo $data; ?>');">
Query string = <?php echo $_SERVER['QUERY_STRING']; ?><br>
Data = <?php echo $data; ?>
</body>
</html>