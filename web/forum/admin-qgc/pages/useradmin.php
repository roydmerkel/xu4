<?php if(!defined("PHORUM_ADMIN")) return; ?>

<?php check_security(); ?>

<?php

if($subaction == "") {

  $pagelength=50;

  if(!$sort) {

    $sort='username';

  }

  $order = " order by $sort";



  $SQL="select id,email,username from $pho_main"."_auth";

  if($where){

    $SQL.=" where username like '%$where%' or email like '%$where%'";

  }

  $SQL.=$order;



  $q->query($DB, $SQL);



  $users_found=$q->numrows();



  $maxpages=$users_found/$pagelength;

  if($maxpages > intval($maxpages))

    $maxpages=intval(++$maxpages);



?>

<script language="JavaScript" type="text/javascript">



function deluser(url){



  ans=window.confirm("You are about to delete this user.  Do you want to continue?");

  if(ans){

    window.location.replace(url);

  }

}

</script>

<?php

    $i=0;

  $j=0;

 if(!$st)

   $st=0;

    while($row=$q->getrow()) {
      $i++;
      if(($i-1)<$st)
         continue;

      if(($i-1)>=($st+$pagelength))
          break;

      $users[$j]=$row;
      $j++;
    }
    $real_users = $j;



    $SQL="select count(*) as count from $pho_main"."_auth";

    $q->query($DB, $SQL);

    $row=$q->getrow();

    $total_users=$row['count'];

    if($st==0) {

      $backlink="&nbsp;";

    } else {

      $backlink="<a href=\"$myname?page=useradmin&sort=$sort&where=$where&st=".($st-$pagelength)."\">back</a>";

    }

    if(($st+$pagelength)>=$total_users) {

      $forwardlink="&nbsp;";

    } else {

      $forwardlink="<a href=\"$myname?page=useradmin&sort=$sort&where=$where&st=".($st+$pagelength)."\">forward</a>";

    }



    $page=$st/$pagelength+1;

?>

<br />

<table width="600" border="0" cellspacing="0" cellpadding="3">

<tr>

    <td nowrap width="50%" align="left" valign="middle"><a href="<?php echo $myname."?page=useradmin&subaction=adduser&sort=$sort&st=$st";?>">Add User</a><br />showing <?php echo $real_users; ?> of <?php echo $total_users; ?> total users</td>

    <td nowrap width="50%" align="right" valign="middle"><form style="align: right; display: inline;" action="<?php echo $myname; ?>" method="get"><input type="hidden" name="page" value="useradmin" /><input type="text" name="where" size="20" value="<?=$where?>" /><input type="submit" value="Search" /></form></td>

</tr>

</table>

<hr width="600"/>

<table width="600" border="0" cellspacing="0" cellpadding="3">

<tr>

    <td nowrap width="10%" align="center" valign="middle"><?php echo $backlink;?></td>

    <td nowrap width="80%" align="center" valign="middle"><center><?php print "Page $page of $maxpages";?></center></td>

    <td nowrap width="10%" align="center" valign="middle"><?php echo $forwardlink;?></td>

</tr>

</table>

<table width="600" border="0" cellspacing="0" cellpadding="3" class="box-table">

<tr>

<td class="table-header"><a href="<?php echo $myname."?page=useradmin&where=$where&sort=username"?>">Username</a></td>

<td class="table-header"><a href="<?php echo $myname."?page=useradmin&where=$where&sort=email"?>">EMail</a></td>

<td class="table-header">Action</td>

</tr>

<?php

$i=0;

while($row=$users[$i]) {

?>

<tr>

<td valign="middle"><?php echo htmlspecialchars($row['username']);?></td>

<td valign="middle"><?php echo htmlspecialchars($row['email']);?></td>

<td valign="middle"><a href="javascript:deluser('<?php echo $myname."?page=useradmin&action=useradmin&subaction=delete&sort=$sort&st=$st&uid=".$row['id'];?>')">delete</a>&nbsp;&nbsp;|&nbsp;&nbsp;<a href="<?php echo $myname."?page=useradmin&subaction=edituser&sort=$sort&st=$st&uid=".$row['id'];?>">edit</a></td>

</tr>

<?php

$i++;

   }

?>

</table>



<?php

} elseif($subaction=="edituser" || $subaction=="adduser") {

    if($subaction=="edituser"){

        $user_id=$uid;

        $SQL="Select * from $pho_main"."_auth where id='$user_id'";

        $q->query($DB, $SQL);

        $rec=$q->getrow();

        if(!is_array($rec))

          $error=$lNoUser;

        if(!$submit)

          $sig=$rec['signature'];

    }

?>

<SCRIPT LANGUAGE="JavaScript">

    function textlimit(field, limit) {

        if (field.value.length > limit)

            field.value = field.value.substring(0, limit);

    }

</script>

<form action="<?php echo $myname; ?>" method="post">

<input type="hidden" name="page" value="useradmin">

<input type="hidden" name="action" value="useradmin">

<input type="hidden" name="subaction" value="save_user">

<input type="hidden" name="uid" value="<?php echo $uid; ?>">

<input type="hidden" name="st" value="<?php echo $st; ?>">

<input type="hidden" name="sort" value="<?php echo $sort; ?>">



<table cellspacing="0" cellpadding="2" border="0" class="box-table" width="600">

<tr>

    <td class="table-header" colspan="2"><?php echo ($subaction=="edituser") ? "Edit User Profile" : "Add User"; ?> :</td>

</tr>

<tr>

    <th nowrap>&nbsp;Username:&nbsp;&nbsp;</th>

    <td><input type="text" name="username" size="30" maxlength="50" value="<?php echo htmlspecialchars($rec['username']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;Name:&nbsp;&nbsp;</th>

    <td><input type="text" name="name" size="30" maxlength="50" value="<?php echo htmlspecialchars($rec['name']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;Email:&nbsp;&nbsp;</th>

    <td ><input type="text" name="email" size="30" maxlength="50" value="<?php echo htmlspecialchars($rec['email']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;Password:&nbsp;&nbsp;<br>(input to change)</th>

    <td ><input type="password" name="password" size="30" maxlength="20" value=""></td>

</tr>

<tr>

    <th nowrap>&nbsp;Password (repetition): </th>

    <td ><input type="password" name="password2" size="30" maxlength="20" value=""></td>

</tr>

<tr>

    <th nowrap>&nbsp;Webpage:&nbsp;&nbsp;</th>

    <td ><input type="text" name="webpage" size="50" maxlength="100" value="<?php echo htmlspecialchars($rec['webpage']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;Image:&nbsp;&nbsp;</th>

    <td ><input type="text" name="image" size="50" maxlength="100" value="<?php echo htmlspecialchars($rec['image']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;ICQ:&nbsp;&nbsp;</th>

    <td ><input type="text" name="icq" size="50" maxlength="50" value="<?php echo htmlspecialchars($rec['icq']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;AOL:&nbsp;&nbsp;</th>

    <td ><input type="text" name="aol" size="50" maxlength="50" value="<?php echo htmlspecialchars($rec['aol']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;Yahoo:&nbsp;&nbsp;</th>

    <td ><input type="text" name="yahoo" size="50" maxlength="50" value="<?php echo htmlspecialchars($rec['yahoo']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;MSN:&nbsp;&nbsp;</th>

    <td ><input type="text" name="msn" size="50" maxlength="50" value="<?php echo htmlspecialchars($rec['msn']); ?>"></td>

</tr>

<tr>

    <th nowrap>&nbsp;Jabber:&nbsp;&nbsp;</th>

    <td ><input type="text" name="jabber" size="50" maxlength="50" value="<?php echo htmlspecialchars($rec['jabber']); ?>"></td>

</tr>

<tr>

    <th valign=top nowrap>&nbsp;Signature:&nbsp;&nbsp;</th>

    <td ><textarea onKeyDown="textlimit(this.form.signature,255);" onKeyUp="textlimit(this.form.signature,255);" cols="30" rows="6" name="signature"><?php echo "\n".$rec['signature']; ?></textarea></td>

</tr>

<tr>

    <th nowrap>&nbsp;Permission-Level:&nbsp;&nbsp;</th>

    <td ><input type="text" name="permission_level" size="50" maxlength="50" value="<?php echo $rec['permission_level']; ?>"></td>

</tr>

<?php

$SQL="Select forum_id from $PHORUM[mod_table] where user_id=$user_id";

$q->query($DB, $SQL);

while($row=$q->getrow()) {

  $moderation[$row[forum_id]]=true;

}

?>

<tr>

    <th valign=top nowrap>&nbsp;Phorum Admin:&nbsp;&nbsp;</th>

    <td><input type="checkbox" name="grant_admin" value="1" <?php if($moderation[0]==true) echo 'checked'?>> Allow full access to the admin.</td>

</tr>

</table>

<br /><br />

<table cellspacing="0" cellpadding="2" border="0" width="600" class="box-table">

<tr>

    <td class="table-header" colspan="5">Moderation</td>

</tr>

<?php

    $SQL="Select id, name from $PHORUM[main_table] order by name";

    $q->query($DB, $SQL);

    $rec=$q->getrow();

    while($rec["id"]){

        echo "<tr>\n";

        for($x=0;$x<5;$x++){

            if(!empty($rec["id"])){

            echo "<td><input type=\"checkbox\" name=\"grant_mod[]\" value=\"$rec[id]\"";

        if($moderation[$rec[id]]==true)

          echo 'checked';

        echo "> $rec[name]</td>";

            }

            else {

                echo "<td>&nbsp;</td>";

            }

            $rec=$q->getrow();

        }

        echo "</tr>\n";

    }

?>

</table>

<br /><br />

<input type="submit" name="submit" value="Save">

</form>

<?php } ?>

