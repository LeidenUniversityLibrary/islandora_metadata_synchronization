<?php

/**
 * @file
 * Contain form functions for editing an object's sync id.
 */

/**
 * Object properties form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object whose properties this form will modify.
 *
 * @return array
 *   The drupal form definition.
 */

function number_of_sources() {
  // how many sources can be defined in the admin form
  return 3;
}

/**
* Implements hook_form().
**/

function ubl_metadata_synchronization_settings_form(array $form, array &$form_state) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');

   $form = array();

   $form['ubl_metadata_synchronization_process_fieldset'] = array(
     '#type' => 'fieldset',
     '#title' => t('Process synchronization'),
   );
   $form['ubl_metadata_synchronization_process_fieldset']['last_synchronization_date'] = array(
     '#type' => 'textfield',
     '#title' => t('Last synchronization date'),
     '#default_value' => variable_get('ubl_metadata_synchronization_last_synchronization_date'),
     '#description' => t('Use a date with the following format: YYYY-MM-DD'),
   );
   $form['ubl_metadata_synchronization_process_fieldset']['synchronize_start'] = array(
       '#type' => 'submit',
       '#value' => t('Start synchronization'),
       '#submit' => array('ubl_metadata_synchronization_start_sync_submit'),
     );

   $form['ubl_metadata_synchronization_sources_fieldset'] = array(
     '#type' => 'fieldset',
     '#title' => t('Sources'),
     '#description' => t('The metadata synchronization can use multiple sources. When synchronizing, each source is tried in order. If a valid identifier is found and metadata can be retrieved with it using the retrieval type and url, other sources are skipped. In the synchronize page of a specific item you can see which source is used.'),
   );
   for ($i = 1; $i <= number_of_sources(); $i++) {
     $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i] = array(
        '#type' => 'fieldset',
        '#title' => t('Source '.$i),   
     );
     if (isset($form_state['input']['source'.$i.'_retrieval_url']) && strlen($form_state['input']['source'.$i.'_retrieval_url']) > 0) {
       $retrievalurl = $form_state['input']['source'.$i.'_retrieval_url'];
     }
     else { 
       $retrievalurl = variable_get('ubl_metadata_synchronization_source'.$i.'_retrieval_url');
     }
     $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i]['source'.$i.'_retrieval_url'] = array(
        '#type' => 'textfield',
        '#title' => t('Retrieval URL'),
        '#description' => t('URL of an end point that is capable of supplying data.'),
        '#default_value' => $retrievalurl,
     );
     if (isset($retrievalurl) && strlen($retrievalurl) > 0) {
       $repoName = ubl_metadata_synchronization_test_oai_pmh_url($retrievalurl);
       if (isset($repoName) && strlen($repoName) > 0) {
         $markup = format_string('<img src="@image_url"/>!message', array(
          '@image_url' => url('misc/watchdog-ok.png'),
          '!message' => t('Successfully connected to OAI-PMH server named @name.', array( '@name' => $repoName )),
        ));
       }
       else {
         $markup = format_string('<img src="@image_url"/>!message', array(
          '@image_url' => url('misc/watchdog-error.png'),
          '!message' => t('The URL above is not a valid URL for an OAI-PMH server.'),
        ));
       }
       $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i]['infobox'] = array(
         '#type' => 'item',
         '#markup' => $markup,
       );
     }

     $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i]['source'.$i.'_set'] = array(
        '#type' => 'textfield',
        '#title' => t('Set'),
        '#default_value' => variable_get('ubl_metadata_synchronization_source'.$i.'_set'),
        '#description' => t('The sepcific set to use at this end point, leave empty to use no set.'),
     );

     $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i]['source'.$i.'_id_prefix'] = array(
        '#type' => 'textfield',
        '#title' => t('Prefix ID'),
        '#default_value' => variable_get('ubl_metadata_synchronization_source'.$i.'_id_prefix'),
        '#description' => t('The default prefix that is used if the identifier appears to be missing the prefix.'),
     );
     $desc = t('The datastream that is used to retrieve the ID for synchronization');
     if ($i == 1) {
       $desc .= t(' for example: MODS');
     }
     if ($i == 2) {
       $desc .= t(' for example: SYNCID');
     }
     $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i]['source'.$i.'_dsid'] = array(
        '#type' => 'textfield',
        '#title' => t('Datastream ID'),   
        '#default_value' => variable_get('ubl_metadata_synchronization_source'.$i.'_dsid'),
        '#description' => $desc,
     );
     $desc = t('The XPath to search the datastream that is used to retrieve the ID for synchronization');
     if ($i == 1) {
       $desc .= t(' for example: /mods:mods/mods:recordInfo/mods:recordIdentifier[@source=\'NL-LeU\']');
     }
     if ($i == 2) {
       $desc .= t(' for example leave blank, if used with datastream ID SYNCID.');
     }
     $form['ubl_metadata_synchronization_sources_fieldset']['group'.$i]['source'.$i.'_path'] = array(
        '#type' => 'textfield',
        '#title' => t('XPath ID'),   
        '#default_value' => variable_get('ubl_metadata_synchronization_source'.$i.'_path'),
        '#description' => $desc,
     );
   }
   $form['ubl_metadata_synchronization_sources_fieldset']['submit'] = array(
       '#type' => 'submit',
       '#value' => t('Save configuration'),
       '#submit' => array('ubl_metadata_synchronization_settings_form_submit'),
     );


   return $form;
}

/**
* Implements hook_form_validate().
**/

function ubl_metadata_synchronization_settings_form_validate(array $form, array &$form_state) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');
  module_load_include('inc','ubl_metadata_synchronization','includes/objects_identifiers');
  for ($i = 1; $i <= number_of_sources(); $i++) {
    $dsid = $form_state['values']['source'.$i.'_dsid'];
    $path = $form_state['values']['source'.$i.'_path'];
    
    if (strlen($form_state['values']['source'.$i.'_retrieval_url']) > 0) {
      $repoName = ubl_metadata_synchronization_test_oai_pmh_url($form_state['values']['source'.$i.'_retrieval_url']);
      if($repoName == FALSE) {
        form_set_error('source'.$i.'_retrieval_url', t('Must be a valid OAI-PMH URL!'));
      }
      else {
        //drupal_set_message(t('OAI-PMH responded with name: @name', array('@name' => $repoName)));
      }
    
      if (strlen($dsid) == 0 && strlen($path) > 0) {
        form_set_error('source'.$i.'_path', t('Source '.$i.' has an xpath but not a datastream id'));
      }
      else if (strlen($path) > 0) {
        // check xpath
        $doc = new DOMDocument;
        $xpath = ubl_metadata_synchronization_new_xpath($doc);
  
        if(@$xpath->query($path) === false) {
          form_set_error('source'.$i.'_path', t('Source '.$i.' has an invalid xpath'));
        }
      }
    }
    else {
      if (strlen($path) > 0) {
        form_set_error('source'.$i.'_path', t('Source '.$i.' has an xpath but not a retrieval url'));
      }  
      if (strlen($dsid) > 0) {
        form_set_error('source'.$i.'_path', t('Source '.$i.' has a datastream id but not a retrieval url'));
      }  
    }
  }
}

/**
* Implements hook_form_submit().
**/

function ubl_metadata_synchronization_settings_form_submit(array $form, array &$form_state) {
  for ($i = 1; $i <= number_of_sources(); $i++) {
    variable_set('ubl_metadata_synchronization_source'.$i.'_retrieval_url', $form_state['values']['source'.$i.'_retrieval_url']);
    variable_set('ubl_metadata_synchronization_source'.$i.'_set', $form_state['values']['source'.$i.'_set']);
    variable_set('ubl_metadata_synchronization_source'.$i.'_id_prefix', $form_state['values']['source'.$i.'_id_prefix']);
    variable_set('ubl_metadata_synchronization_source'.$i.'_dsid', $form_state['values']['source'.$i.'_dsid']);
    variable_set('ubl_metadata_synchronization_source'.$i.'_path', $form_state['values']['source'.$i.'_path']);
  }
  drupal_set_message(t('The settings have been saved!'));
}

/**
* Implements hook_form_submit().
**/

function ubl_metadata_synchronization_start_sync_submit(array $form, array &$form_state) {
  module_load_include('inc','ubl_metadata_synchronization','includes/synchronize');
  $date = $form_state['values']['last_synchronization_date'];

  ubl_metadata_synchronization_start_sync(array('date' => $date));
}

/**
* Implements hook_form().
**/

function sync_id_editor_form(array $form, array &$form_state, AbstractObject $object) {
  module_load_include('inc','ubl_metadata_synchronization','includes/objects_identifiers');
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');

  $form_state['object'] = $object;

  $source_with_url = NULL;
  $sources_with_url = array();
  $sync_source = NULL;
  $saved_sync_id = '';
  if (isset($object['SYNCID'])) {
    $saved_sync_id = $object['SYNCID']->content;
  }
  $form = array(
    'pid' => array(
      '#type' => 'hidden',
      '#value' => $object->id,
    ),
  );
  $form['ubl_metadata_synchronization_sync_item_sources'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sources'),
  );
  for ($i = 1; $i <= number_of_sources(); $i++) {
    $retrieveurl = variable_get('ubl_metadata_synchronization_source'.$i.'_retrieval_url');
    $idprefix = variable_get('ubl_metadata_synchronization_source'.$i.'_id_prefix');
    $dsid = variable_get('ubl_metadata_synchronization_source'.$i.'_dsid');
    $xpath = variable_get('ubl_metadata_synchronization_source'.$i.'_path');

    if (isset($retrieveurl) && strlen($retrieveurl) > 0) {
      if (!isset($source_with_url)) {
        $source_with_url = $i;
      }
      $sources_with_url[$i] = $i;
      $foundxml = NULL;
      $foundid = ubl_metadata_synchronization_get_identifier($object,$dsid,$xpath);
    
      if (isset($foundid)) {
        $xml = ubl_metadata_synchronization_retrieve_oaipmh_metadata($retrieveurl,$foundid,$idprefix);
        if ($xml !== FALSE && strlen($xml) > 0) {
          $foundxml = $xml;
          if (!isset($sync_source)) {
            $sync_source = $i;
          }
        } 
      }

      $markup = '<DIV>Source '.$i.': ';
      if (isset($foundid)) {
        $markup .= 'Identifier '.$foundid.' found in datastream '.$dsid; 
        if (isset($foundxml)) {
          $markup .= ' that can be used to retrieve metadata from '.$retrieveurl.'.';      
        }
        else {
          $markup .= ' but no metadata was found for it.';      
        }
      }
      else {
        $markup .= 'No identifier found';
        if (isset($xpath) && strlen($xpath) > 0) {
          $markup .= ' for xpath '.$xpath;
        }
        $markup .= ' in datastream '.$dsid.'.'; 
      }
      $markup .= '</DIV>';
      $form['ubl_metadata_synchronization_sync_item_sources']['source'.$i] = array(
         '#type' => 'item',
         '#markup' => $markup,
      ); 
    }
  }  
  $form['sync_source'] = array(
    '#type' => 'hidden',
    '#value' => $sync_source,
  );
  $form['source_url_nr'] = array(
    '#type' => 'hidden',
    '#value' => $source_with_url,
  );
  $syncbuttontitle = ($sync_source > 0)?t('Synchronize now with source !source', array('!source' => $sync_source)):t('Cannot synchronize');
  $form['ubl_metadata_synchronization_sync_item_sources']['synchronize'] = array(
    '#type' => 'submit',
    '#value' => $syncbuttontitle,
    '#submit' => array('sync_id_editor_form_synchronize_submit'),
    '#disabled' => !($sync_source > 0),
  );
  $form['ubl_metadata_synchronization_sync_item_set_identifier'] = array(
    '#type' => 'fieldset',
    '#title' => t('Synchronize or set specific identifier'),
  );
  $form['ubl_metadata_synchronization_sync_item_set_identifier']['sync_id'] = array(
    '#title' => t('Synchronization Identifier'),
    '#default_value' => $saved_sync_id,
    '#description' => t('The identifier of the metadata to synchronize with. Leave empty if this identifier can be found in the metadata of this object.'),
    // Double the normal length.
    '#size' => 120,
    '#type' => 'textfield',
  );
  for ($i = 1; $i <= number_of_sources(); $i++) {
    $form['ubl_metadata_synchronization_sync_item_set_identifier']['sync_w_syncid'.$i] = array(
      '#type' => 'submit',
      '#value' => t('Synchronize with this Sync ID and source !source', array('!source' => $i)),
      '#submit' => array('sync_id_editor_form_synchronize_with_syncid_submit'),
      '#name' => 'syncid_'.$i,
      '#disabled' => !($sources_with_url[$i] > 0),
    );
  }
  $form['ubl_metadata_synchronization_sync_item_set_identifier']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Set Sync ID'),
    '#submit' => array('sync_id_editor_form_set_sync_id_submit'),
  );
  return $form;
}

/**
* Implements the form submit to store a given sync id for 1 record
**/

function sync_id_editor_form_set_sync_id_submit(array $form, array &$form_state) {
  $object = $form_state['object'];
  $syncid = $form_state['values']['sync_id'];

  if (isset($object['SYNCID'])) {
    if ((!isset($syncid)) || strlen($syncid) == 0) {
      unset($object['SYNCID']); 
    }
    else if ($object['SYNCID']->content !== $syncid) {
      $object['SYNCID']->content = $syncid;
    }
  }
  else {
    $syncid_datastream = $object->constructDatastream('SYNCID');
    $syncid_datastream->label = 'Sync ID';
    $syncid_datastream->mimetype = 'text/plain';
    $syncid_datastream->setContentFromString($syncid);
    $object->ingestDatastream($syncid_datastream);
  }

  drupal_set_message("Sync ID set");
}

/**
* Implements the form submit to synchronize 1 record based on a sbumitted identifier
**/

function sync_id_editor_form_synchronize_with_syncid_submit(array $form, array &$form_state) {
  module_load_include('inc','ubl_metadata_synchronization','includes/synchronize');
  $object = $form_state['object'];
  $syncid = $form_state['values']['sync_id'];

  if (strlen($syncid) == 0) {
    drupal_set_message("specify a Sync ID",'error'); 
    return;
  }
  $clickedname = $form_state['clicked_button']['#name'];
  preg_match('/^syncid_(\d+)$/',$clickedname,$matches); 
  $sourceurlnr = $matches[1] ?: 0;
  $retrieveurl = variable_get('ubl_metadata_synchronization_source'.$sourceurlnr.'_retrieval_url');

  $result = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$retrieveurl,$syncid);

  if ($result) {
    drupal_set_message($result,'error');
  }
  else {
    drupal_set_message(t("Synchronization successful!"));
  }
}

/**
* Implements the form submit to synchronize 1 record that has it's own identifier stored in the metadata
**/

function sync_id_editor_form_synchronize_submit(array $form, array &$form_state) {
  module_load_include('inc','ubl_metadata_synchronization','includes/synchronize');
  $object = $form_state['object'];
  $syncsource = $form_state['values']['sync_source'];
  
  $retrieveurl = variable_get('ubl_metadata_synchronization_source'.$syncsource.'_retrieval_url');
  $idprefix = variable_get('ubl_metadata_synchronization_source'.$syncsource.'_id_prefix');
  $dsid = variable_get('ubl_metadata_synchronization_source'.$syncsource.'_dsid');
  $xpath = variable_get('ubl_metadata_synchronization_source'.$syncsource.'_path');

  $syncid = ubl_metadata_synchronization_get_identifier($object,$dsid,$xpath);

  $result = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$retrieveurl,$syncid,$idprefix);

  if ($result) {
    drupal_set_message($result,'error');
  }
  else {
    drupal_set_message(t("Synchronization successful!"));
  }
}
