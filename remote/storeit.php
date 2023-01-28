<?php
$file=fopen("data.txt","w");
fwrite($file,$_SERVER['QUERY_STRING']);
fclose($file);
?>
<html><body>Done</body></html>
