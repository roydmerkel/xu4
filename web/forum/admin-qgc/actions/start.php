<?php if(!defined("PHORUM_ADMIN")) return; ?>
<?php check_security(); ?>
<?php
  $PHORUM["started"]=1;
  writefile();
  $step=0;
  QueMessage("Phorum has been started.");
?>