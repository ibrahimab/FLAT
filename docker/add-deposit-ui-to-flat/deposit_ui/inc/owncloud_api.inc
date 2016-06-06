<?php
define('OC_ADMIN', variable_get('owncloud_settings',array())['admin_name']);
define('OC_PASS', variable_get('owncloud_settings',array())['admin_pass']);
define('OC_ROOT', variable_get('owncloud_settings',array())['root_dir']);
#define('OC_HOST', variable_get('owncloud_settings',array())['host']);
define('OC_HOST', $_SERVER['HTTP_HOST']);

class OwnCloudConnectionException extends Exception {}

class OwnCloud {

    /* owncloud credentials
     * @var admin:password
     *
    */
    public $admin_name;
    public $admin_pass;
    public $root_dir;

    /* Should owncloud be used or not
    *
    */
    public $inUse;

    /* Check variable indicating owncloud is functioning
    *
    */
    public $ok;

    /* user name
    *
    */
    public $user_name;

    /* Constructor function
    *
    */
    public function __construct($user = NULL) {
        $this->admin_name = OC_ADMIN;
        $this->admin_pass = OC_PASS;
        $this->root_dir = OC_ROOT;

        if (!isset($user)) {
            global $user;
            if ($user->uid == 0) {
                $this->user_name = 'anonymous';
            }
            else {
                $this->user_name = $user->name;
            }
        }

        $this->inUse = variable_get('flat_deposit_modules',array())['owncloud'];

        // if owncloud is used check for existence account; else always true
        if ($this->inUse){

            $this->ok = self::owncloud_account_exists();
        }
        // otherwise check is always true
        else {
            $this->ok = True;
        }
    }

    public function owncloud_account_exists()
    {
        $oc_url = 'http://' . $this->admin_name . ':' . $this->admin_pass . '@' . OC_HOST . '/owncloud/ocs/v1.php/cloud/users/' . $this->user_name;

        $options = array(
            'max_redirects' => 3,);

        $result = drupal_http_request($oc_url, $options);

        try {
            if (owncloud_check_connection($result)) {
                $xml = new SimpleXMLElement(file_get_contents($oc_url));

                if (owncloud_check_user($xml)){
                    return true;
                } else {
                    throw new OwnCloudConnectionException("Owncloud user account doesn't exist. Login into owncloud account or ask the administrator");
                }
            }else{
                throw new OwnCloudConnectionException('Owncloud server error code (' . $result->code  . ')');}
        }

        catch (OwnCloudConnectionException $e) {
            drupal_set_message(t('Check for owncloud account failed. Error message: %message', array('%message' => $e->getMessage())),'error');
            watchdog('Owncloud account', t('Owncloud account not available'), array(), WATCHDOG_ERROR);
            return false;
    }



    }

    public function UpdateUserFiles()
    {
        if ($this->inUse && $this->ok) {
            $occ_command = 'php ' . $this->root_dir . '/occ  files:scan --path="/' . $this->user_name . '/files"';
            shell_exec($occ_command);
        }
    }

    public function CreateLinkAndSync($target, $project){

        if ($this->inUse && $this->ok) {
            $link_name = $this->root_dir . 'data/' . $this->user_name . '/files/' . $project;
            symlink (drupal_realpath($target) .'/data', $link_name);
            $this::UpdateUserFiles();
        }
    }
    public function DeleteLinkAndSync($project){

        if ($this->inUse && $this->ok) {
            $link_name = $this->root_dir . 'data/' . $this->user_name . '/files/' . $project;

            if(is_link($link_name)) {
                drupal_unlink($link_name);
            } else {
                drupal_set_message(t("$link_name exists but not symbolic link\n", array()), 'error');
            }
            $this::UpdateUserFiles();
        }
    }

}


function owncloud_check_connection($result) {
    if ($result->code == "200") {
        return true;
    } else {
        return false;
    }
}

function owncloud_check_user($xml)
{
    if ($xml->data->enabled) {
        return true;
    } else {
        return false;
    }
}


