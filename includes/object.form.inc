<?php

/**
 * @file
 * Contains form functions for syncing one object with a specific source and id
 */


/**
 * Calculate how many sources are defined in the admin form.
 */
function number_of_sources() {
  $nr_of_srcs = 1;
  for ($i = 1; $i <= 99; $i++) {
    $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $nr_of_srcs++;
    }
    else {
      break;
    }
  }

  return $nr_of_srcs;
}

/**
 * Implements hook_form().
 */
function sync_id_editor_form(array $form, array &$form_state, AbstractObject $object) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/objects_identifiers');
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/retrieval_oaipmh');

  $form_state['object'] = $object;

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
  $form['islandora_metadata_synchronization_sync_item_sources'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sources'),
  );
  for ($i = 1; $i <= number_of_sources(); $i++) {
    $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    $idprefix = variable_get('islandora_metadata_synchronization_source' . $i . '_id_prefix');
    $dsid = variable_get('islandora_metadata_synchronization_source' . $i . '_id_dsid');
    $xpath = variable_get('islandora_metadata_synchronization_source' . $i . '_path');
    $mdprefix = variable_get('islandora_metadata_synchronization_source' . $i . '_mdprefix');

    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $sources_with_url[$i] = $i;
      $foundxml = NULL;
      $foundid = islandora_metadata_synchronization_get_identifier($object, $dsid, $xpath);

      if (isset($foundid)) {
        $xml = islandora_metadata_synchronization_retrieve_oaipmh_metadata($oaipmh_url, $foundid, $mdprefix);
        if ($xml !== FALSE && strlen($xml) > 0) {
          $foundxml = $xml;
          if (!isset($sync_source)) {
            $sync_source = $i;
          }
        }
      }

      $markup = '<DIV>Source ' . $i . ': ';
      if (isset($foundid)) {
        $markup .= 'Identifier ' . $foundid . ' found in datastream ' . $dsid;
        if (isset($foundxml)) {
          $markup .= ' that can be used to retrieve metadata from ' . $oaipmh_url . '.';
        }
        else {
          $markup .= ' but no metadata was found for that identifier with metadata prefix ' . $mdprefix . '.';
        }
      }
      else {
        $markup .= 'No identifier found';
        if (isset($xpath) && strlen($xpath) > 0) {
          $markup .= ' for xpath ' . $xpath;
        }
        $markup .= ' in datastream ' . $dsid . '.';
      }
      $markup .= '</DIV>';
      $form['islandora_metadata_synchronization_sync_item_sources']['source' . $i] = array(
        '#type' => 'item',
        '#markup' => $markup,
      );
    }
  }
  $form['sync_source'] = array(
    '#type' => 'hidden',
    '#value' => $sync_source,
  );
  $syncbuttontitle = ($sync_source > 0) ? t('Synchronize now with source !source', array('!source' => $sync_source)) : t('Cannot synchronize');
  $form['islandora_metadata_synchronization_sync_item_sources']['synchronize'] = array(
    '#type' => 'submit',
    '#value' => $syncbuttontitle,
    '#submit' => array('sync_id_editor_form_synchronize_submit'),
    '#disabled' => !($sync_source > 0),
  );
  $form['islandora_metadata_synchronization_sync_item_set_identifier'] = array(
    '#type' => 'fieldset',
    '#title' => t('Synchronize or set specific identifier'),
  );
  $form['islandora_metadata_synchronization_sync_item_set_identifier']['sync_id'] = array(
    '#title' => t('Synchronization Identifier'),
    '#default_value' => $saved_sync_id,
    '#description' => t('The identifier of the metadata to synchronize with. Leave empty if this identifier can be found in the metadata of this object.'),
    // Double the normal length.
    '#size' => 120,
    '#type' => 'textfield',
  );
  foreach ($sources_with_url as $i) {
    $form['islandora_metadata_synchronization_sync_item_set_identifier']['sync_w_syncid' . $i] = array(
      '#type' => 'submit',
      '#value' => t('Synchronize with this Sync ID and source !source', array('!source' => $i)),
      '#submit' => array('sync_id_editor_form_synchronize_with_syncid_submit'),
      '#name' => 'syncid_' . $i,
    );
  }
  $form['islandora_metadata_synchronization_sync_item_set_identifier']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Set Sync ID'),
    '#submit' => array('sync_id_editor_form_set_sync_id_submit'),
  );
  return $form;
}

/**
 * Implements the form submit to store a given sync id for 1 record.
 */
function sync_id_editor_form_set_sync_id_submit(array $form, array &$form_state) {
  $object = $form_state['object'];
  $syncid = $form_state['values']['sync_id'];

  if (isset($object['SYNCID'])) {
    if ((!isset($syncid)) || strlen($syncid) == 0) {
      unset($object['SYNCID']);
    }
    elseif ($object['SYNCID']->content !== $syncid) {
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

  drupal_set_message(t("Sync ID set"));
}

/**
 * Implements the form submit to synchronize 1 record based on submitted id.
 */
function sync_id_editor_form_synchronize_with_syncid_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/synchronize');
  $object = $form_state['object'];
  $syncid = $form_state['values']['sync_id'];

  if (strlen($syncid) == 0) {
    drupal_set_message(t("specify a Sync ID"), 'error');
    return;
  }
  $clickedname = $form_state['clicked_button']['#name'];
  preg_match('/^syncid_(\d+)$/', $clickedname, $matches);
  $sourceurlnr = $matches[1] ?: 0;
  $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $sourceurlnr . '_oaipmh_url');
  $idprefix = variable_get('islandora_metadata_synchronization_source' . $sourceurlnr . '_id_prefix');
  $mdprefix = variable_get('islandora_metadata_synchronization_source' . $sourceurlnr . '_mdprefix');
  $mdxsl = variable_get('islandora_metadata_synchronization_source' . $sourceurlnr . '_mdxsl');
  $dsid = variable_get('islandora_metadata_synchronization_source' . $sourceurlnr . '_target_dsid');

  if (isset($idprefix) && strlen($idprefix) > 0) {
    if (substr($syncid, 0, strlen($idprefix)) !== $idprefix) {
      $syncid = $idprefix . $syncid;
    }
  }

  $result = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaipmh_url, $syncid, $mdprefix, $mdxsl, $dsid);

  if ($result) {
    drupal_set_message(check_plain($result), 'error');
  }
  else {
    drupal_set_message(t("Synchronization successful!"));
  }
}

/**
 * Implements the form submit to synchronize 1 record with id from metadata.
 */
function sync_id_editor_form_synchronize_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/synchronize');
  $object = $form_state['object'];
  $syncsource = $form_state['values']['sync_source'];

  $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_oaipmh_url');
  $idprefix = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_id_prefix');
  $iddsid = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_id_dsid');
  $path = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_path');
  $mdprefix = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_mdprefix');
  $mdxsl = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_mdxsl');
  $dsid = variable_get('islandora_metadata_synchronization_source' . $syncsource . '_target_dsid');

  $syncid = islandora_metadata_synchronization_get_identifier($object, $iddsid, $path);

  if (isset($idprefix) && strlen($idprefix) > 0) {
    if (substr($syncid, 0, strlen($idprefix)) !== $idprefix) {
      $syncid = $idprefix . $syncid;
    }
  }

  $result = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaipmh_url, $syncid, $mdprefix, $mdxsl, $dsid);

  if ($result) {
    drupal_set_message(check_plain($result), 'error');
  }
  else {
    drupal_set_message(t("Synchronization successful!"));
  }
}
