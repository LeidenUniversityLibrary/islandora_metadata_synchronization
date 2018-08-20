<?php

/**
 * @file
 * functions for synchronizing metadat with an external source.
 *
 *
 *  Copyright 2017 Leiden University Library
 *
 *  This file is part of islandora_metadata_synchronization.
 *
 *  islandora_metadata_synchronization is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/**
 * Synchronize the metadata via OAI-PMH for a specific object.
 */
function islandora_metadata_synchronization_sync_metadata_for_object_using(AbstractObject $object, $oaipmh_url, $syncid, $mdprefix, $mdxsl, $addmdxsl = NULL, $dsid = 'MODS', $force = FALSE) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/retrieval_oaipmh');
  $oaimdxml = islandora_metadata_synchronization_retrieve_oaipmh_metadata($oaipmh_url, $syncid, $mdprefix);

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
    $handle = islandora_metadata_synchronization_get_handle($object);
    if ($handle) {
      $parameters['handle'] = $handle;
    }
  }
  $xml = islandora_metadata_synchronization_xsl_transform($oaimdxml, $mdxsl, $parameters);

  if (!$xml) {
    return t("Failed to do primary transformation for @dsid of object @objid with sync id @syncid",
      array(
        "@dsid" => $dsid,
        "@objid" => $object->id,
        "@syncid" => $syncid,
      )
    );
  }

  if (isset($addmdxsl) && strlen($addmdxsl) > 0) {
    $xml = islandora_metadata_synchronization_xsl_transform($xml, $addmdxsl, $parameters);
  
    if (!$xml) {
      return t("Failed to do secondary transformation for @dsid of object @objid with sync id @syncid",
        array(
          "@dsid" => $dsid,
          "@objid" => $object->id,
          "@syncid" => $syncid,
        )
      );
    }
  }

  $ischanged = islandora_metadata_synchronization_save_xml($object, $xml, $dsid, "$dsid XML", $force);

  if ($ischanged && ($dsid === 'MODS')) {
    $dcxml = islandora_metadata_synchronization_create_dc_from_mods($xml);

    if (!$dcxml) {
      return t("Could not create DC for object @objid with sync id @syncid",
        array(
          "@objid" => $object->id,
          "@syncid" => $syncid,
        )
      );
    }

    islandora_metadata_synchronization_save_xml($object, $dcxml, 'DC', 'DC Record');
  }

  if ($ischanged) {
    if (module_exists('rules')) {
      rules_invoke_event('islandora_metadata_synchronization_object_synchronized', $object);
    }
  }

  return FALSE;
}

/**
 * Save XML to the DSID of a specific object, set label of DSID.
 */
function islandora_metadata_synchronization_save_xml(AbstractObject $object, $xml, $dsid, $label, $force = FALSE) {
  if (isset($object[$dsid])) {
    if ($force || ($object[$dsid]->content !== $xml)) {
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
function islandora_metadata_synchronization_xsl_transform($xml, $xslfile, $parameters = array()) {
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
function islandora_metadata_synchronization_create_dc_from_mods($modsxml) {
  // TODO: use the configured mods_to_dc transformation for this.
  $xsl = drupal_get_path('module', 'islandora_metadata_synchronization') . '/xsl/mods_to_dc.xsl';

  return islandora_metadata_synchronization_xsl_transform($modsxml, $xsl);
}

/**
 * Return an array of available sources.
 */
function islandora_metadata_synchronization_sources($sourceid = NULL) {
  $sources = array();

  for ($i = 1; $i <= 99; $i++) {
    if (($sourceid != NULL) && ($sourceid != $i)) {
      continue;
    }
    $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $set = variable_get('islandora_metadata_synchronization_source' . $i . '_set');
      $mdprefix = variable_get('islandora_metadata_synchronization_source' . $i . '_mdprefix');
      $idprefix = variable_get('islandora_metadata_synchronization_source' . $i . '_id_prefix');
      $iddsid = variable_get('islandora_metadata_synchronization_source' . $i . '_id_dsid');
      $targetdsid = variable_get('islandora_metadata_synchronization_source' . $i . '_target_dsid');
      $path = variable_get('islandora_metadata_synchronization_source' . $i . '_path');
      $mdxsl = variable_get('islandora_metadata_synchronization_source' . $i . '_mdxsl');
      $addmdxsl = variable_get('islandora_metadata_synchronization_source' . $i . '_addmdxsl');

      $sources[$i] = array(
        'oaipmh_url' => $oaipmh_url,
        'set'          => $set,
        'mdprefix'     => $mdprefix,
        'id_prefix'    => $idprefix,
        'id_dsid'      => $iddsid,
        'target_dsid'  => $targetdsid,
        'path'         => $path,
        'mdxsl'        => $mdxsl,
        'addmdxsl'     => $addmdxsl,
      );
    }
  }
  return $sources;
}

/**
 * Start synchronization using all sources.
 */
function islandora_metadata_synchronization_start_sync($params, $usingdrush = FALSE, $mapping = NULL) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/retrieval_oaipmh');

  $oaiurls = array();

  for ($i = 1; $i <= 99; $i++) {
    $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    $set = variable_get('islandora_metadata_synchronization_source' . $i . '_set');
    $mdprefix = variable_get('islandora_metadata_synchronization_source' . $i . '_mdprefix');
    $dsid = variable_get('islandora_metadata_synchronization_source' . $i . '_target_dsid');
    $mdxsl = variable_get('islandora_metadata_synchronization_source' . $i . '_mdxsl');
    $addmdxsl = variable_get('islandora_metadata_synchronization_source' . $i . '_addmdxsl');
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $key = $oaipmh_url;
      if (isset($set)) {
        $key .= '_' . $set;
      }
      if (!isset($oaiurls[$key])) {
        $newparams = $params;
        $newparams['set'] = $set;
        $newparams['mdprefix'] = $mdprefix;
        $newparams['target_dsid'] = $dsid;
        $newparams['mdxsl'] = $mdxsl;
        $newparams['addmdxsl'] = $addmdxsl;
        islandora_metadata_synchronization_start_sync_with_url($oaipmh_url, $newparams, $usingdrush, $mapping);
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
function islandora_metadata_synchronization_start_sync_with_url($oaiurl, $params, $usingdrush = FALSE, $mapping = NULL) {
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
    islandora_metadata_synchronization_handle_batch($oaiurl, $params, array(), $usingdrush, $mapping, $cntxt);
  }
  else {
    $batch = array(
      'file' => drupal_get_path('module', 'islandora_metadata_synchronization') . '/includes/synchronize.inc',
      'operations' => array(
        array('islandora_metadata_synchronization_handle_batch',
          array($oaiurl, $params, array(), $usingdrush, $mapping),
        ),
        array('islandora_metadata_synchronization_almost_ready',
          array(),
        ),
      ),
      'finished' => 'islandora_metadata_synchronization_finished',
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
function islandora_metadata_synchronization_map_identifier($an_id, $mapping) {
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
function islandora_metadata_synchronization_handle_batch($oaiurl, $params, $results, $usingdrush, $mapping, &$context) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/retrieval_oaipmh');

  $oairesult = islandora_metadata_synchronization_identifiers_for_changed_metadata($oaiurl, $params);

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
  $addmdxsl = $params['addmdxsl'];
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
  foreach ($oairesult['ids'] as $oaiid) {
    $object = find_fedora_object_for_id($oaiid);
    if (!$object) {
      $an_id = islandora_metadata_synchronization_map_identifier($oaiid, $mapping);
      if ($an_id !== $oaiid) {
        $object = find_fedora_object_for_id($an_id);
      }
    }
    if ($object) {
      $t = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaiurl, $oaiid, $mdprefix, $mdxsl, $addmdxsl, $dsid);
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
    $newparams = array('resumptiontoken' => $oairesult["resumptiontoken"], 'mdxsl' => $mdxsl, 'addmdxsl' => $addmdxsl, 'target_dsid' => $dsid, 'mdprefix' => $mdprefix);
    if ($usingdrush) {
      islandora_metadata_synchronization_handle_batch($oaiurl, $newparams, $results, $usingdrush, $mapping, $context);
    }
    else {
      // Add an extra batch to handle the resumption token.
      $batch = &batch_get();
      $batch_next_set = $batch['current_set'] + 1;
      $batch_set = &$batch['sets'][$batch_next_set];

      $batch_set['operations'][] = array('islandora_metadata_synchronization_handle_batch',
        array($oaiurl, $newparams, $results, $usingdrush, $mapping),
      );
      $batch_set['operations'][] = array('islandora_metadata_synchronization_almost_ready',
        array(),
      );

      $batch_set['file'] = drupal_get_path('module', 'islandora_metadata_synchronization') . '/includes/synchronize.inc';
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
      variable_set('islandora_metadata_synchronization_last_synchronization_date', $date);
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
function islandora_metadata_synchronization_almost_ready() {
  // Just a "filler" function.
}

/**
 * Finish the synchronization.
 */
function islandora_metadata_synchronization_finished($success, $results, $operations) {
  // Finished.
  if (module_exists('rules')) {
    rules_invoke_event('islandora_metadata_synchronization_selective_objects_synchronized');
  }
}

/**
 * Retrieve an object for a specific id.
 */
function find_fedora_object_for_id($identifier) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_solr', 'includes/query_processor');
  module_load_include('inc', 'islandora_solr', 'includes/utilities');

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

  $qp->buildAndExecuteQuery($search, NULL, FALSE);

  $r = $qp->islandoraSolrResult;

  $num_found = $r['response']['numFound'];

  if ($num_found == 1) {
    $objid = $r['response']['objects'][0]['PID'];
    $obj = islandora_object_load($objid);

    return $obj;
  }
  if ($num_found > 1) {
    watchdog('islandora_metadata_synchronization',
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
function islandora_metadata_synchronization_get_handle($object) {
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
