<?php

function ubl_metadata_synchronization_sync_metadata_for_object_using(AbstractObject $object, $retrievalurl, $syncid, $idprefix = NULL) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');

  $marcxml = ubl_metadata_synchronization_retrieve_oaipmh_metadata($retrievalurl,$syncid, $idprefix);

  if (!$marcxml) {
    return t("No metadata found for object @objid with sync id @syncid", array("@objid" => $object->id, "@syncid" => $syncid));
  }
  
  ubl_metadata_synchronization_save_xml($object,$marcxml,'MARC','MARC XML');

  $modsxml = ubl_metadata_synchronization_create_mods_from_marc($marcxml);

  if (!$modsxml) {
    return t("Could not create MODS for object @objid with sync id @syncid", array("@objid" => $object->id, "@syncid" => $syncid));
  }
  
  $ismodschanged = ubl_metadata_synchronization_save_xml($object,$modsxml,'MODS','MODS XML');

  if ($ismodschanged) {
    $dcxml = ubl_metadata_synchronization_create_dc_from_mods($modsxml);

 
    if (!$dcxml) {
      return t("Could not create DC for object @objid with sync id @syncid", array("@objid" => $object->id, "@syncid" => $syncid));
    }
  
    ubl_metadata_synchronization_save_xml($object,$dcxml,'DC','DC Record');
  }

  return FALSE; 
}

function ubl_metadata_synchronization_save_xml(AbstractObject $object,$xml,$dsid,$label) {
  if (isset($object[$dsid])) {
    if ($object[$dsid]->content !== $xml) {
      $object[$dsid]->content = $xml;

      return TRUE;
    }
  }
  else {
    $xml_datastream = $object->constructDatastream($dsid);
    $xml_datastream->label = $label;
    $xml_datastream->mimetype = 'text/xml';
    $xml_datastream->setContentFromString($xml);
    $object->ingestDatastream($xml_datastream);
   
    return TRUE;
  }
  return FALSE;
}

function ubl_metadata_synchronization_create_mods_from_marc($marcxml) {
  module_load_include('inc','islandora_marcxml','includes/utilities');

  $transform_args = array(
    'xsl' => drupal_get_path('module', 'islandora_marcxml') . '/xsl/MARC21slim2MODS3-5.xsl',
    'input' => $marcxml,
  );
  $transformed_xml = trim(islandora_marcxml_run_xsl_transform($transform_args));

  return $transformed_xml;
}

function ubl_metadata_synchronization_create_dc_from_mods($modsxml) {
  module_load_include('inc','islandora_marcxml','includes/utilities');

  $transform_args = array(
    'xsl' => drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl',
    'input' => $modsxml,
  );
  $transformed_xml = trim(islandora_marcxml_run_xsl_transform($transform_args));

  return $transformed_xml;
}

function ubl_metadata_synchronization_start_sync_from_date($date = NULL, $usingdrush = FALSE, $mapping = null) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');

  $oaiurls = array();

  for ($i = 1; $i <= 99; $i++) {
    $retrieveurl = variable_get('ubl_metadata_synchronization_source'.$i.'_retrieval_url');
    if (isset($retrieveurl) && strlen($retrieveurl) > 0) {
      $oaiurls[$retrieveurl] = $retrieveurl;
    }
    else {
      break;
    }
  }
  if (count($oaiurls) == 0) {
    if ($usingdrush) {
      drush_log(t("No OAI-PMH url found"), "error");
    }
    else {
      drupal_set_message(t("No OAI-PMH url found"), "error");
    }
    return;
  }
  if (count($oaiurls) > 1) {
    if ($usingdrush) {
      drush_log(t("Only use the first OAI-PMH url of the sources"), "warning");
    }
    else {
      drupal_set_message(t("Only use the first OAI-PMH url of the sources"));
    }
  }
  $oaiurl = array_shift($oaiurls);
 
  ubl_metadata_synchronization_start_sync_with_url_from_date($oaiurl, $date, $usingdrush, $mapping);
}

function ubl_metadata_synchronization_start_sync_with_url_from_date($oaiurl, $date = NULL, $usingdrush = FALSE, $mapping = null) {
  $batch = array(
    'file' => drupal_get_path('module', 'ubl_metadata_synchronization') . '/includes/synchronize.inc',
    'operations' => array(
      array('ubl_metadata_synchronization_handle_batch', array($oaiurl,$date,NULL,array(), $usingdrush, $mapping)),
      array('ubl_metadata_synchronization_almost_ready', array()),
    ),
    'finished' => 'ubl_metadata_synchronization_finished',
    'title' => t('Processing'),
    'progress_message' => '',
    'error_message' => t('Synchronization has encountered an error.'),
  );

  if ($usingdrush) {
    $batch['progressive'] = FALSE;
  }
  batch_set($batch);
  
  if ($usingdrush) {
    drush_backend_batch_process();
  }
  else {
    batch_process();
  }
}

function ubl_metadata_synchronization_handle_batch($oaiurl,$date = NULL, $resumptionToken = NULL, $results, $usingdrush, $mapping, &$context) {
  module_load_include('inc','ubl_metadata_synchronization','includes/retrieval_oaipmh');

  $oairesult = ubl_metadata_synchronization_identifiers_for_changed_metadata($oaiurl,$date,$resumptionToken);

  if (array_key_exists('oaierror',$oairesult)) {
    if (($oairesult['oaierror'] === 'no record match the search criteria') && ! array_key_exists('oaiidscount',$results)) {
      $text = t("No records available to synchronize; all up-to-date!");
      if ($usingdrush) {
        drush_log($text, 'ok');
      }
      else {
        $batch = &batch_get();
        $batch_next_set = $batch['current_set'] + 1;
        $batch_set = &$batch['sets'][$batch_next_set];
        array_pop($batch_set['operations']);
        drupal_set_message($text);
      }
    }
    else {
      $text = t('Error while retrieving records: @error', array('@error' => $oairesult['oaierror']));
      if ($usingdrush) {
        drush_log($text, 'warning');
      }
      else {
        $batch = &batch_get();
        $batch_next_set = $batch['current_set'] + 1;
        $batch_set = &$batch['sets'][$batch_next_set];
        array_pop($batch_set['operations']);
        drupal_set_message($text, 'warning');
      }
    }
    return;
  }

  if ( ! array_key_exists('oairesponsedate',$results)) {
    $results['oairesponsedate'] = $oairesult['responsedate'];
  }
  if ( ! array_key_exists('oaiidscount',$results)) {
    $results['oaiidscount'] = 0;
  }
  if ( ! array_key_exists('syncsuccesses',$results)) {
    $results['syncsuccesses'] = 0;
  }
  if ( ! array_key_exists('syncerrors',$results)) {
    $results['syncerrors'] = array();
  }
  $results['oaiidscount'] += count($oairesult['ids']);
  foreach ($oairesult['ids'] as $anId) {
    if ($mapping != null) {
      if (array_key_exists('_pattern', $mapping) && array_key_exists('_replacement', $mapping)) {
        $anId = preg_replace($mapping['_pattern'],$mapping['_replacement'],$anId);
      }
      else {
        if (array_key_exists($anId, $mapping)) {
          $anId = $mapping[$anId];
        }
      }
    }
    $object = find_fedora_object_for_id($anId);
    if ($object) {
      $t = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$oaiurl,$anId);    
      if ($t) {
        $results['syncerrors'][] = $t;
        drush_log(t("Fedora object found for @id but update failed: @error", array('@id' => $anId, '@error' => $t)),'warning');
      }
      else {
        $results['syncsuccesses'] += 1;
        drush_log(t("Fedora object found and is up-to-date for @id", array('@id' => $anId)),'notice');
      }
    }
    else {
      drush_log(t("No fedora object found for @id", array('@id' => $anId)),'notice');
    }
  }
  if (array_key_exists("resumptiontoken",$oairesult)) {
    if ($usingdrush) {
      ubl_metadata_synchronization_handle_batch($oaiurl,NULL,$oairesult["resumptiontoken"],$results,$usingdrush,$mapping,$context);
    }
    else {
      // add an extra batch to handle the resumption token
      $batch = &batch_get();
      $batch_next_set = $batch['current_set'] + 1;
      $batch_set = &$batch['sets'][$batch_next_set];
  
      $batch_set['operations'][] = array('ubl_metadata_synchronization_handle_batch', array($oaiurl,NULL,$oairesult["resumptiontoken"],$results));
      $batch_set['operations'][] = array('ubl_metadata_synchronization_almost_ready', array());
  
      $batch_set['file'] = drupal_get_path('module', 'ubl_metadata_synchronization') . '/includes/synchronize.inc';
      $batch_set['total'] = count($batch_set['operations']);
      $batch_set['count'] = $batch_set['total'];
      $batch_set['success'] = FALSE;
      $batch_set['finished'] = 0.5;
      $ending = '.';
      if (count($results['syncerrors'])) {
        $ending = t(", @errorcount errors", array("@errorcount" => count($results['syncerrors'])));
      }
      $batch_set['init_message'] = t("Processed @oaicount OAI records, @syncsuccesscount records successfully sync'ed@ending",
                                       array("@oaicount" => $results['oaiidscount'],
                                             "@syncsuccesscount" => $results['syncsuccesses'],
                                             "@ending" => $ending));
      $batch_set['progress_message'] = $batch_set['init_message'];
      //$batch_set['success'] = 0;

      _batch_populate_queue($batch, $batch_next_set);
    }
  }
  else {
    if ($usingdrush) {
      // nothing special to do
    }
    else {
      $batch = &batch_get();
      $batch_next_set = $batch['current_set'] + 1;
      $batch_set = &$batch['sets'][$batch_next_set];
      array_pop($batch_set['operations']);
    }  

    $date = $results['oairesponsedate'];
    if ($date) {
      variable_set('ubl_metadata_synchronization_last_synchronization_date', $date);
      $text = t('Successfully synchronized records @syncedcount out of @totalcount, with @errorcount errors',
                    array('@syncedcount' => $results['syncsuccesses'],
                          '@errorcount' => count($results['syncerrors']),
                          '@totalcount' => $results['oaiidscount']));
      if ($usingdrush) {
        drush_log($text, (count($results['syncerrors']) == 0)?'ok':'warning');
      }
      else {
        drupal_set_message($text, (count($results['syncerrors']) == 0)?'ok':'warning');
      }
    }
  }
}

function ubl_metadata_synchronization_almost_ready() {
  // just a "filler" function
}

function ubl_metadata_synchronization_finished($success, $results, $operations)  {
  // finished
}

function find_fedora_object_for_id($id) {
  module_load_include('inc','islandora_solr_search','includes/query_processor');
  module_load_include('inc','islandora_solr_search','includes/utilities');
  module_load_include('inc','islandora');

  $qp = new IslandoraSolrQueryProcessor();
  $escapedid = islandora_solr_lesser_escape($id);
  $search = "catch_all_fields_mt:$escapedid";

  $qp->buildAndExecuteQuery($search); 

  $r = $qp->islandoraSolrResult;

  $numFound = $r['response']['numFound'];

  if ($numFound == 1) {
    $objid = $r['response']['objects'][0]['PID'];
    $obj = islandora_object_load($objid);

    return $obj;
  }
  if ($numFound > 1) {
    watchdog('ubl_metadata_synchronization',
      'Found more than 1 record (@numFound records) with identifier @id.',
      array(
        '@id' => $id,
        '@numFound' => $numFound,
      ),
      WATCHDOG_WARNING
    );
  }

  return FALSE;
}

