<?php
$file=fopen("data.txt","r");
$data=fgets($file);
fclose($file);
?>
<html>
<head>
<script>
function setDivText(msg)
{
  window.parent.postMessage(msg, "*");
}
</script>
</head>
<body onload="setDivText('<?php echo $data; ?>');">
</body>
</html>