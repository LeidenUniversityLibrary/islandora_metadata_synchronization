<?php

/**
 * @file
 * functions for synchronizing metadat with an external source.
 */


/**
 * Synchronize the metadata via OAI-PMH for a specific object.
 */
function ubl_metadata_synchronization_sync_metadata_for_object_using(AbstractObject $object, $oaipmh_url, $syncid, $mdprefix, $mdxsl, $dsid = 'MODS') {
  module_load_include('inc', 'ubl_metadata_synchronization', 'includes/retrieval_oaipmh');
  $oaimdxml = ubl_metadata_synchronization_retrieve_oaipmh_metadata($oaipmh_url, $syncid, $mdprefix);

  if (!$oaimdxml) {
    return t("No metadata found for object @objid via @url with sync id @syncid",
      array(
        "@objid" => $object->id,
        "@url" => $oaipmh_url,
        "@syncid" => $syncid,
      )
    );
  }

  $parameters = array();
  if ($dsid === 'MODS') {
    // Keep the handle that is stored in the MODS datastream.
    $handle = ubl_metadata_synchronization_get_handle($object);
    if ($handle) {
      $parameters['handle'] = $handle;
    }
  }
  $xml = ubl_metadata_synchronization_xsl_transform($oaimdxml, $mdxsl,$parameters);

  if (!$xml) {
    return t("Could not create @dsid for object @objid with sync id @syncid",
      array(
        "@dsid" => $dsid,
        "@objid" => $object->id,
        "@syncid" => $syncid,
      )
    );
  }

  $ischanged = ubl_metadata_synchronization_save_xml($object, $xml, $dsid, "$dsid XML");

  if ($ischanged && ($dsid === 'MODS')) {
    $dcxml = ubl_metadata_synchronization_create_dc_from_mods($xml);

    if (!$dcxml) {
      return t("Could not create DC for object @objid with sync id @syncid",
        array(
          "@objid" => $object->id,
          "@syncid" => $syncid,
        )
      );
    }

    ubl_metadata_synchronization_save_xml($object, $dcxml, 'DC', 'DC Record');
  }

  if ($ischanged) {
    if (module_exists('rules')) {
      rules_invoke_event('ubl_metadata_synchronization_object_synchronized', $object);
    }
  }

  return FALSE;
}

/**
 * Save XML to the DSID of a specific object, set label of DSID.
 */
function ubl_metadata_synchronization_save_xml(AbstractObject $object, $xml, $dsid, $label) {
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

/**
 * Transform XML as a string by using XSL as a file.
 */
function ubl_metadata_synchronization_xsl_transform($xml, $xslfile, $parameters = array()) {
  $xsl = new DOMDocument();
  $xsl->load($xslfile);
  $input = new DOMDocument();
  $input->loadXML($xml);
  $processor = new XSLTProcessor();
  $processor->importStylesheet($xsl);

  foreach ($parameters as $parkey => $parvalue) {
    $processor->setParameter('',$parkey,$parvalue);
  }

  // XXX: Suppressing warnings regarding unregistered prefixes.
  return trim(@$processor->transformToXML($input));
}

/**
 * Create DC as a string from the MODS XML as a string.
 */
function ubl_metadata_synchronization_create_dc_from_mods($modsxml) {
  module_load_include('inc', 'islandora_marcxml', 'includes/utilities');

  $xsl = drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl';

  return ubl_metadata_synchronization_xsl_transform($modsxml, $xsl);
}

/**
 * Return an array of available sources.
 */
function ubl_metadata_synchronization_sources($sourceid = NULL) {
  $sources = array();

  for ($i = 1; $i <= 99; $i++) {
    if (($sourceid != NULL) && ($sourceid != $i)) {
      continue;
    }
    $oaipmh_url = variable_get('ubl_metadata_synchronization_source' . $i . '_oaipmh_url');
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $set = variable_get('ubl_metadata_synchronization_source' . $i . '_set');
      $mdprefix = variable_get('ubl_metadata_synchronization_source' . $i . '_mdprefix');
      $idprefix = variable_get('ubl_metadata_synchronization_source' . $i . '_id_prefix');
      $iddsid = variable_get('ubl_metadata_synchronization_source' . $i . '_id_dsid');
      $targetdsid = variable_get('ubl_metadata_synchronization_source' . $i . '_target_dsid');
      $path = variable_get('ubl_metadata_synchronization_source' . $i . '_path');
      $mdxsl = variable_get('ubl_metadata_synchronization_source' . $i . '_mdxsl');

      $sources[$i] = array(
        'oaipmh_url' => $oaipmh_url,
        'set'          => $set,
        'mdprefix'     => $mdprefix,
        'id_prefix'    => $idprefix,
        'id_dsid'      => $iddsid,
        'target_dsid'  => $targetdsid,
        'path'         => $path,
        'mdxsl'        => $mdxsl,
      );
    }
  }
  return $sources;
}

/**
 * Start synchronization using all sources.
 */
function ubl_metadata_synchronization_start_sync($params, $usingdrush = FALSE, $mapping = NULL) {
  module_load_include('inc', 'ubl_metadata_synchronization', 'includes/retrieval_oaipmh');

  $oaiurls = array();

  for ($i = 1; $i <= 99; $i++) {
    $oaipmh_url = variable_get('ubl_metadata_synchronization_source' . $i . '_oaipmh_url');
    $set = variable_get('ubl_metadata_synchronization_source' . $i . '_set');
    $mdprefix = variable_get('ubl_metadata_synchronization_source' . $i . '_mdprefix');
    $mdxsl = variable_get('ubl_metadata_synchronization_source' . $i . '_mdxsl');
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $key = $oaipmh_url;
      if (isset($set)) {
        $key .= '_' . $set;
      }
      if (!isset($oaiurls[$key])) {
        $newparams = $params;
        $newparams['set'] = $set;
        $newparams['mdprefix'] = $mdprefix;
        $newparams['mdxsl'] = $mdxsl;
        ubl_metadata_synchronization_start_sync_with_url($oaipmh_url, $newparams, $usingdrush, $mapping);
        $oaiurls[$key] = $i;
      }
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
  $t = t("Synchronized with the following sources: @sources", array("@sources" => implode(" and ", $oaiurls)));
  if ($usingdrush) {
    drush_log($t, "ok");
  }
  else {
    drupal_set_message($t);
  }
}

/**
 * Start synchronization using a specific source.
 */
function ubl_metadata_synchronization_start_sync_with_url($oaiurl, $params, $usingdrush = FALSE, $mapping = NULL) {
  if ($usingdrush) {
    $setdesc = '';
    if (isset($params["set"]) && strlen($params["set"]) > 0) {
      $setdesc = ' ' . t("and set '@set'", array("@set" => $params["set"]));
    }
    drush_log(t("Starting sync with url '@url'!set.",
                array(
                  "@url" => $oaiurl,
                  "!set" => $setdesc,
                )
              ));
    $cntxt = array();
    ubl_metadata_synchronization_handle_batch($oaiurl, $params, array(), $usingdrush, $mapping, $cntxt);
  }
  else {
    $batch = array(
      'file' => drupal_get_path('module', 'ubl_metadata_synchronization') . '/includes/synchronize.inc',
      'operations' => array(
        array('ubl_metadata_synchronization_handle_batch',
          array($oaiurl, $params, array(), $usingdrush, $mapping),
        ),
        array('ubl_metadata_synchronization_almost_ready',
          array(),
        ),
      ),
      'finished' => 'ubl_metadata_synchronization_finished',
      'title' => t('Processing'),
      'progress_message' => '',
      'error_message' => t('Synchronization has encountered an error.'),
    );

    batch_set($batch);

    batch_process();
  }
}

/**
 * Map an identifier to another identifier using the mapping.
 */
function ubl_metadata_synchronization_map_identifier($an_id, $mapping) {
  if ($mapping != NULL) {
    if (isset($mapping['_pattern']) && isset($mapping['_replacement'])) {
      $an_id = preg_replace($mapping['_pattern'], $mapping['_replacement'], $an_id);
    }
    else {
      if (isset($mapping[$an_id])) {
        $an_id = $mapping[$an_id];
      }
    }
  }
  return $an_id;
}

/**
 * Handle the batch synchronization of the metadata.
 */
function ubl_metadata_synchronization_handle_batch($oaiurl, $params, $results, $usingdrush, $mapping, &$context) {
  module_load_include('inc', 'ubl_metadata_synchronization', 'includes/retrieval_oaipmh');

  $oairesult = ubl_metadata_synchronization_identifiers_for_changed_metadata($oaiurl, $params);

  if (isset($oairesult['oaierror'])) {
    if (($oairesult['oaierror'] === 'no record match the search criteria') && !isset($oairesult['oaiidscount'])) {
      $text = t("No records available to synchronize; all up-to-date!");
      if ($usingdrush) {
        drush_log($text, 'ok');
      }
      else {
        $batch = &batch_get();
        $batch_next_set = $batch['current_set'] + 1;
        $batch_set = &$batch['sets'][$batch_next_set];
        array_pop($batch_set['operations']);
        drupal_set_message(check_plain($text));
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
        if (isset($batch_set['operations'])) {
          array_pop($batch_set['operations']);
        }
        drupal_set_message($text, 'warning');
      }
    }
    return;
  }

  $mdxsl = $params['mdxsl'];
  $mdprefix = $params['mdprefix'];
  $dsid = $params['target_dsid'];

  if (!isset($results['oairesponsedate'])) {
    $results['oairesponsedate'] = $oairesult['responsedate'];
  }
  if (!isset($results['oaiidscount'])) {
    $results['oaiidscount'] = 0;
  }
  if (!isset($results['syncsuccesses'])) {
    $results['syncsuccesses'] = 0;
  }
  if (!isset($results['syncerrors'])) {
    $results['syncerrors'] = array();
  }
  $results['oaiidscount'] += count($oairesult['ids']);
  foreach ($oairesult['ids'] as $an_id) {
    $an_id = ubl_metadata_synchronization_map_identifier($an_id, $mapping);
    $object = find_fedora_object_for_id($an_id);
    if ($object) {
      $t = ubl_metadata_synchronization_sync_metadata_for_object_using($object, $oaiurl, $an_id, $mdprefix, $mdxsl, $dsid);
      if ($t) {
        $results['syncerrors'][] = $t;
        if ($usingdrush) {
          drush_log(t("Fedora object found for @id but update failed: @error", array('@id' => $an_id, '@error' => $t)), 'warning');
        }
      }
      else {
        $results['syncsuccesses'] += 1;
        if ($usingdrush) {
          drush_log(t("Fedora object found and is up-to-date for @id", array('@id' => $an_id)), 'notice');
        }
      }
    }
    else {
      if ($usingdrush) {
        drush_log(t("No fedora object found for @id", array('@id' => $an_id)), 'notice');
      }
    }
  }
  if (isset($oairesult["resumptiontoken"])) {
    $newparams = array('resumptiontoken' => $oairesult["resumptiontoken"], 'mdxsl' => $mdxsl);
    if ($usingdrush) {
      ubl_metadata_synchronization_handle_batch($oaiurl, $newparams, $results, $usingdrush, $mapping, $context);
    }
    else {
      // Add an extra batch to handle the resumption token.
      $batch = &batch_get();
      $batch_next_set = $batch['current_set'] + 1;
      $batch_set = &$batch['sets'][$batch_next_set];

      $batch_set['operations'][] = array('ubl_metadata_synchronization_handle_batch',
        array($oaiurl, $newparams, $results, $usingdrush, $mapping),
      );
      $batch_set['operations'][] = array('ubl_metadata_synchronization_almost_ready',
        array(),
      );

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
        array(
          "@oaicount" => $results['oaiidscount'],
          "@syncsuccesscount" => $results['syncsuccesses'],
          "@ending" => $ending,
        )
      );
      $batch_set['progress_message'] = $batch_set['init_message'];

      _batch_populate_queue($batch, $batch_next_set);
    }
  }
  else {
    if ($usingdrush) {
      // Nothing special to do.
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
                array(
                  '@syncedcount' => $results['syncsuccesses'],
                  '@errorcount' => count($results['syncerrors']),
                  '@totalcount' => $results['oaiidscount'],
                )
              );
      if ($usingdrush) {
        drush_log($text, (count($results['syncerrors']) == 0) ? 'ok' : 'warning');
      }
      else {
        drupal_set_message($text, (count($results['syncerrors']) == 0) ? 'status' : 'warning');
      }
    }
  }
}

/**
 * Just a "filler" function.
 */
function ubl_metadata_synchronization_almost_ready() {
  // Just a "filler" function.
}

/**
 * Finish the synchronization.
 */
function ubl_metadata_synchronization_finished($success, $results, $operations) {
  // Finished.
  if (module_exists('rules')) {
    rules_invoke_event('ubl_metadata_synchronization_selective_objects_synchronized');
  }
}

/**
 * Retrieve an object for a specific id.
 */
function find_fedora_object_for_id($identifier) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_solr_search', 'includes/query_processor');
  module_load_include('inc', 'islandora_solr_search', 'includes/utilities');

  if (islandora_is_valid_pid($identifier)) {
    // This looks like a islandora object id.

    $obj = islandora_object_load($identifier);

    if ($obj) {
      return $obj;
    }
  }
  static $qp = NULL;
  static $qpc = 0;
  if (is_null($qp) || $qpc > 10) {
    // Get a fresh copy the first time and after each 10 queries.
    // Apparently there are limitations on the amount of queries it can handle.
    $qp = new IslandoraSolrQueryProcessor();
    $qpc = 0;
  }
  $qpc++;
  $escapedid = islandora_solr_lesser_escape($identifier);
  $search = "catch_all_fields_mt:$escapedid";

  $qp->buildAndExecuteQuery($search);

  $r = $qp->islandoraSolrResult;

  $num_found = $r['response']['numFound'];

  if ($num_found == 1) {
    $objid = $r['response']['objects'][0]['PID'];
    $obj = islandora_object_load($objid);

    return $obj;
  }
  if ($num_found > 1) {
    watchdog('ubl_metadata_synchronization',
      'Found more than 1 record (@numFound records) with identifier @id.',
      array(
        '@id' => $identifier,
        '@numFound' => $num_found,
      ),
      WATCHDOG_WARNING
    );
  }

  return FALSE;
}

/**
 * Retrieve the handle for this object.
 */
function ubl_metadata_synchronization_get_handle($object) {
  $handle = FALSE;
  if (isset($object['MODS'])) {
    $xpath = "/mods:mods/mods:identifier[@type='hdl']";
    $content = $object['MODS']->content;
    $domdoc = new DOMDocument();
    if ($domdoc->loadXML($content)) {
      $domxpath = new DOMXPath($domdoc);
      $domxpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
      $domnodelist = $domxpath->query($xpath);
      if ($domnodelist->length > 0) {
        foreach ($domnodelist as $domnode) {
          $text = $domnode->textContent;
          if (isset($text) && strlen($text) > 0) {
            $handle = $text;
            break;
          }
        }
      }
    }
  }
  return $handle;
}
