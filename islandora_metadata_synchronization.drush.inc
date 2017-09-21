<?php

/**
 * @file
 * Functions for synchronization via drush.
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
 * Implements hook_drush_command().
 */
function islandora_metadata_synchronization_drush_command() {
  $items['start_metadata_synchronization'] = array(
    'description' => 'Retrieve the metadata that was changed after a specified date from the specified source and synchronize it with the found records in Islandora.',
    'options' => array(
      'source' => 'The source id as configured in islandora/tools/islandora_metadata_synchronization.',
      'date' => 'The date after which the metadata has changed. Format YYYY-MM-DD. Enter \'last\' to use the last synchronization date.',
      'mapping_pattern' => 'Optional, but use in conjunction with mapping_replacement. Use this to make another mapping for the OAI identifier',
      'mapping_replacement' => 'Optional, but use in conjunction with mapping_pattern. Use this to make another mapping for the OAI identifier',
      'mapping_file' => 'Optional, a file containing a mapping for the identifiers. Each line contains 2 identifiers separated by a tab. The first identifier is the one that is used in the source, the second is the one that is used in islandora.',
      'ids_file' => 'Optional, a file containing a list of identifiers. Only those identifiers will be synchronised. The mapping are tried on those identifiers as well and if they fail, the identifier from the metadata is used with the mapping.',
      'batch_set_id' => 'Optional, a batch set id of a set that was previously ingested succesfully. Only those records will be synchronised.',
    ),
    'aliases' => array('smds'),
    'examples' => array(
      'drush start_metadata_synchronization --source=1 --date=last',
      'drush smds --source=2 --date=2015-12-15',
      'drush smds --source=2 --date=2015-12-15 --mapping_pattern="/oai:archimedes.leidenuniv.nl:/" --mapping_replacement="oai:disc.leidenuniv.nl:"',
      'drush smds --source=2 --date=2015-12-15 --mapping_file=/tmp/mapping_file.txt',
      'drush smds --source=2 --ids_file=/tmp/ids_file.txt',
      'drush smds --source=2 --ids_file=/tmp/ids_file.txt --mapping_file=/tmp/mapping_file.txt',
      'drush smds --source=2 --batch_set_id=6',
      'drush smds --source=2 --batch_set_id=6 --mapping_file=/tmp/mapping_file.txt',
    ),
  );
  return $items;
}

/**
 * Implements drush_hook_COMMAND_validate().
 */
function drush_islandora_metadata_synchronization_start_metadata_synchronization_validate() {
  $sourceid = drush_get_option('source');

  if ($sourceid != NULL) {
    if (preg_match("/^\d+$/", $sourceid) != 1) {
      return drush_set_error("the option source should be a number indicating the source id as configured in islandora/tools/islandora_metadata_synchronization.");
    }
  }

  $date = drush_get_option('date');
  $idsfile = drush_get_option('ids_file');
  $batchsetid = drush_get_option('batch_set_id');
  $checkset = array();
  if ($date !== NULL) { $checkset['date'] = $date; }
  if ($idsfile !== NULL) { $checkset['idsfile'] = $idsfile; }
  if ($batchsetid !== NULL) { $checkset['batchsetid'] = $batchsetid; }
  if (count($checkset) != 1) {
    return drush_set_error("Exactly one of the options date, ids_file of batch_set_id should exist.");
  }
  if ($date != NULL && preg_match("/^(?:last|\d\d\d\d-\d\d-\d\d(?:T\d\d:\d\d:\d\dZ)?)$/", $date) != 1) {
    return drush_set_error("the option date should be the value 'last' or a date formatted as YYYY-MM-DD");
  }
  if ($batchsetid != NULL && preg_match("/^\d+$/", $batchsetid) != 1) {
    return drush_set_error("the option batch_set_id should be a number");
  }

  $mappat = drush_get_option('mapping_pattern');
  $maprep = drush_get_option('mapping_replacement');
  if (($mappat == NULL && $maprep != NULL) || ($mappat != NULL && $maprep == NULL)) {
    return drush_set_error("The options mapping_pattern and mapping_replacement should be used together, or not at all.");
  }
  if ($mappat != NULL && @preg_match($mappat, "check if pattern is valid regex") === FALSE) {
    return drush_set_error("The pattern '$mappat' is not a valid regular expression");
  }
  $mapfile = drush_get_option('mapping_file');
  if ($mapfile != NULL) {
    if ($mappat != NULL) {
      return drush_set_error("mapping_file cannot be used at the same time as mapping_pattern");
    }
    if (!file_exists($mapfile)) {
      return drush_set_error("mapping_file '$mapfile' does not exist");
    }
  }
  if ($idsfile != NULL) {
    if (!file_exists($idsfile)) {
      return drush_set_error("ids_file '$idsfile' does not exist");
    }
  }
  if ($batchsetid != NULL) {
    module_load_include('inc', 'islandora_batch', 'includes/db');
    $count = islandora_batch_get_count_of_queued_set_objects($batchsetid);
    if ($count == 0) {
      return drush_set_error("batch set id '$batchsetid' does not contain any items");
    }
  }

  return TRUE;
}

/**
 * Implements drush_hook_COMMAND().
 */
function drush_islandora_metadata_synchronization_start_metadata_synchronization() {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/synchronize');
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/objects_identifiers');

  $sourceid = drush_get_option('source');
  $date = drush_get_option('date');
  $mappat = drush_get_option('mapping_pattern');
  $maprep = drush_get_option('mapping_replacement');
  $mapfile = drush_get_option('mapping_file');
  $idsfile = drush_get_option('ids_file');
  $batchsetid = drush_get_option('batch_set_id');

  $mapping = NULL;
  if ($mappat != NULL && $maprep != NULL) {
    $mapping = array("_pattern" => $mappat, "_replacement" => $maprep);
  }
  elseif ($mapfile != NULL) {
    $fieldseparator = "";
    $mapping = array();
    $content = file_get_contents($mapfile);
    $contentarray = str_getcsv($content, "\n");
    foreach ($contentarray as $index => $line) {
      if (strlen($line) > 0) {
        if ($fieldseparator === "") {
          $fieldseparator = "\t";
          $comps = str_getcsv($line, $fieldseparator);
          if (count($comps) != 2) {
            $fieldseparator = ",";
            $comps = str_getcsv($line, $fieldseparator);
          }
          if (count($comps) != 2) {
            $fieldseparator = ";";
          }
        }
        $comps = str_getcsv($line, $fieldseparator);
        if (count($comps) != 2) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber '$line': not 2 components but ".count($comps)." components");
        }
        $key = $comps[0];
        $value = $comps[1];
        if (isset($mapping[$key]) && $mapping[$key] !== $value) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber: source id '$key' maps to 2 different target ids");
        }
        if (isset($mapping[$value]) && $mapping[$value] !== $key) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber: target id '$value' maps to 2 different source ids");
        }
        $mapping[$key] = $value;
        $mapping[$value] = $key;
      }
    }
  }

  if ($date === 'last') {
    $date = variable_get('islandora_metadata_synchronization_last_synchronization_date');
    drush_log("Synchronizing from date $date");
  }

  if ($date !== NULL) {
    $params = array('date' => $date);
    if ($sourceid == NULL) {
      islandora_metadata_synchronization_start_sync($params, TRUE, $mapping);
    }
    else {
      $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $sourceid . '_oaipmh_url');
      $params['set'] = variable_get('islandora_metadata_synchronization_source' . $sourceid . '_set');
      $params['mdxsl'] = variable_get('islandora_metadata_synchronization_source' . $sourceid . '_mdxsl');

      islandora_metadata_synchronization_start_sync_with_url($oaipmh_url, $params, TRUE, $mapping);
    }
  }
  else {
    $sources = islandora_metadata_synchronization_sources($sourceid);
    $foundids = array();

    if ($idsfile !== NULL) {
      $foundids = ids_from_file($idsfile);
    }
    elseif ($batchsetid !== NULL) {
      $foundids = islandora_batch_get_queued_items_for_set($batchsetid); 
    }
    drush_log("Found " . count($foundids). " IDs to synchronize");
    foreach ($foundids as $id) {
      $object = find_fedora_object_for_id($id);
      if ($object) {
        drush_log("Synchronizing object " . $object->id . " found for $id");
        $usedoaiurls = array();
        foreach ($sources as $source) {
          $oaiurl = $source['oaipmh_url'];
          if (isset($usedoaiurls[$oaiurl])) {
            break;
          }
          $usedoaiurls[$oaiurl] = 1;
          $idprefix = $source['id_prefix'];
          $syncid = $id;
          if (isset($idprefix) && strlen($idprefix) > 0) {
            if (substr($syncid, 0, strlen($idprefix)) !== $idprefix) {
              $syncid = $idprefix . $syncid;
            }
          }
          $mdxsl = $source['mdxsl'];
          $mdprefix = $source['mdprefix'];
          $dsid = $source['target_dsid'];

          $t = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaiurl, $syncid, $mdprefix, $mdxsl, $dsid);
          if ($t) {
            $an_id = islandora_metadata_synchronization_map_identifier($id, $mapping);
            if ($id !== $an_id) {
              $syncid = $an_id;
              if (isset($idprefix) && strlen($idprefix) > 0) {
                if (substr($syncid, 0, strlen($idprefix)) !== $idprefix) {
                  $syncid = $idprefix . $syncid;
                }
              }
              $t = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaiurl, $syncid, $mdprefix, $mdxsl, $dsid);
            }
          }
          if ($t) {
            $dsid = $source['id_dsid'];
            $xpath = $source['path'];
            $an_id = islandora_metadata_synchronization_get_identifier($object, $dsid, $xpath);
            if ($an_id != NULL) {
              if ($id !== $an_id) {
                $t = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaiurl, $an_id, $mdprefix, $mdxsl, $dsid);
                if ($t) {
                  $another_id = islandora_metadata_synchronization_map_identifier($an_id, $mapping);
                  $syncid = $another_id;
                  if (isset($idprefix) && strlen($idprefix) > 0) {
                    if (substr($syncid, 0, strlen($idprefix)) !== $idprefix) {
                      $syncid = $idprefix . $syncid;
                    }
                  }
                  if ($an_id !== $syncid) {
                    $t = islandora_metadata_synchronization_sync_metadata_for_object_using($object, $oaiurl, $syncid, $mdprefix, $mdxsl, $dsid);
                  }
                }
              }
            }
          }
          if ($t) {
            drush_log("Cannot synchronize object $id: $t", "warning");
          }
          else {
            drush_log("Successfully synchronized object $id", "ok");
            break;
          }
        }
      }
      else {
        drush_log("Cannot find object $id", "warning");
      }
    }
  }
}

function ids_from_file($idsfile) {
  $foundids = array();

  $idsstr = file_get_contents($idsfile);
  $fieldseparator = "";
  $ids = str_getcsv($idsstr, "\n");
  foreach ($ids as $index => $id) {
    if (strlen($id) == 0) {
      continue;
    }
    if ($fieldseparator === "") {
      $fieldseparator = "\t";
      $comps = str_getcsv($id, $fieldseparator);
      if (count($comps) > 1) {
        // Fields are separated by a tab.
      }
      else {
        $fieldseparator = ",";
        $comps = str_getcsv($id, $fieldseparator);
        if (count($comps) > 1) {
          // Fields are separated by a comma.
        }
        else {
          $fieldseparator = ";";
          $comps = str_getcsv($id, $fieldseparator);
          if (count($comps) > 1) {
            // Fields are separated by a semicolon.
          }
          else {
            // Fields are not separated.
            $fieldseparator = FALSE;
          }
        }
      }
    }
    if ($fieldseparator !== FALSE) {
      $comps = str_getcsv($id, $fieldseparator);
      if (count($comps) > 1) {
        $id = $comps[0];
      }
    }
    $foundids[] = $id;
  }
  return $foundids;
}
