<?php

/**
* Implements hook_drush_command().
**/

function ubl_metadata_synchronization_drush_command() {
  $items['start_metadata_synchronization'] = array(
    'description' => 'Retrieve the metadata that was changed after a specified date from the specified source and synchronize it with the found records in Islandora.',
    'options' => array(
      'source' => 'The source id as configured in islandora/tools/ubl_metadata_synchronization.',
      'date' => 'The date after which the metadata has changed. Format YYYY-MM-DD. Enter \'last\' to use the last synchronization date.',
      'mapping_pattern' => 'Optional, but use in conjunction with mapping_replacement. Use this to make another mapping for the OAI identifier',
      'mapping_replacement' => 'Optional, but use in conjunction with mapping_pattern. Use this to make another mapping for the OAI identifier',
      'mapping_file' => 'Optional, a file containing a mapping for the identifiers. Each line contains 2 identifiers separated by a tab. The first identifier is the one that is used in the source, the second is the one that is used in islandora.',
      'ids_file' => 'Optional, a file containing a list of identifiers. Only those identifiers will be synchronised. The mapping are tried on those identifiers as well and if they fail, the identifier from the metadata is used with the mapping.',
    ),
    'aliases' => array('smds'),
    'examples' => array(
      'drush start_metadata_synchronization --source=1 --date=last',
      'drush smds --source=2 --date=2015-12-15',
      'drush smds --source=2 --date=2015-12-15 --mapping_pattern="/oai:archimedes.leidenuniv.nl:/" --mapping_replacement="oai:disc.leidenuniv.nl:"',
      'drush smds --source=2 --date=2015-12-15 --mapping_file=/tmp/mapping_file.txt',
      'drush smds --source=2 --ids_file=/tmp/ids_file.txt',
      'drush smds --source=2 --ids_file=/tmp/ids_file.txt --mapping_file=/tmp/mapping_file.txt',
    ),
  );
  return $items;
}

/**
* Implements drush_hook_COMMAND_validate().
**/

function drush_ubl_metadata_synchronization_start_metadata_synchronization_validate() {
  $sourceid = drush_get_option('source');

  if ($sourceid != null) {
    if (preg_match("/^\d+$/", $sourceid) != 1) {
      return drush_set_error("the option source should be a number indicating the source id as configured in islandora/tools/ubl_metadata_synchronization.");
    }
  }

  $date = drush_get_option('date');
  $idsfile = drush_get_option('ids_file');
  if (($date != null && $idsfile != null) || ($date == null && $idsfile == null)) {
    return drush_set_error("either option date or option ids_file should exist, not neither and not both");
  }
  if ($date != null && preg_match("/^(?:last|\d\d\d\d-\d\d-\d\d(?:T\d\d:\d\d:\d\dZ)?)$/",$date) != 1) {
    return drush_set_error("the option date is mandatory and should be the value 'last' or a date formatted as YYYY-MM-DD");
  } 

  $mappat = drush_get_option('mapping_pattern');
  $maprep = drush_get_option('mapping_replacement');
  if (($mappat == null && $maprep != null) || ($mappat != null && $maprep == null)) {
    return drush_set_error("The options mapping_pattern and mapping_replacement should be used together, or not at all.");
  }
  if ($mappat != null && @preg_match($mappat,"check if pattern is valid regex") === false) {
    return drush_set_error("The pattern '$mappat' is not a valid regular expression");
  }
  $mapfile = drush_get_option('mapping_file');
  if ($mapfile != null) {
    if ($mappat != null) {
      return drush_set_error("mapping_file cannot be used at the same time as mapping_pattern");
    }
    if ( ! file_exists($mapfile)) {
      return drush_set_error("mapping_file '$mapfile' does not exist");
    }
  }
  if ($idsfile != null) {
    if ( ! file_exists($idsfile)) {
      return drush_set_error("ids_file '$idsfile' does not exist");
    }
  }

  return TRUE;
}

/**
* Implements drush_hook_COMMAND().
**/

function drush_ubl_metadata_synchronization_start_metadata_synchronization() {
  module_load_include('inc','ubl_metadata_synchronization','includes/synchronize');
  module_load_include('inc','ubl_metadata_synchronization','includes/objects_identifiers');

  $sourceid = drush_get_option('source');
  $date = drush_get_option('date');
  $mappat = drush_get_option('mapping_pattern');
  $maprep = drush_get_option('mapping_replacement');
  $mapfile = drush_get_option('mapping_file');
  $idsfile = drush_get_option('ids_file');

  $mapping = null;
  if ($mappat != null && $maprep != null) {
    $mapping = array("_pattern" => $mappat, "_replacement" => $maprep);
  }
  else if ($mapfile != null) {
    $fieldseperator = "\t";
    $mapping = array();
    $content = file_get_contents($mapfile);
    $contentarray = str_getcsv($content,"\n"); 
    foreach ($contentarray as $index => $line) {
      if (strlen($line) > 0) {
        $comps = str_getcsv($line,$fieldseperator);
        if (count($comps) != 2) {
          $fieldseperator = ",";
          $comps = str_getcsv($line,$fieldseperator);
        }
        if (count($comps) != 2) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber: not 2 components");
        }
        $key = $comps[0];
        $value = $comps[1];
        if (isset($mapping[$key]) && $mapping[$key] !== $value) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber: source id '$key' maps to 2 different target ids");
        }
        $mapping[$key] = $value;
      }
    }
  }

  if ($date === 'last') {
    $date = variable_get('ubl_metadata_synchronization_last_synchronization_date');
    drush_log("Synchronizing from date $date");
  }
  
  if ($idsfile == null) {
    $params = array('date' => $date);
    if ($sourceid == null) {
      ubl_metadata_synchronization_start_sync($params,true,$mapping);
    }
    else {
       $oaipmh_url = variable_get('ubl_metadata_synchronization_source'.$sourceid.'_oaipmh_url');
       $params['set'] = variable_get('ubl_metadata_synchronization_source'.$sourceid.'_set');
       $params['mdxsl'] = variable_get('ubl_metadata_synchronization_source'.$sourceid.'_mdxsl');

      ubl_metadata_synchronization_start_sync_with_url($oaipmh_url,$params,true,$mapping);
    }
  }
  else {
    $sources = ubl_metadata_synchronization_sources($sourceid);
    $idsstr = file_get_contents($idsfile);
    $ids = str_getcsv($idsstr,"\n");
    foreach ($ids as $index => $id) {
      if (strlen($id) == 0) {
        continue;
      }
      $object = find_fedora_object_for_id($id);
      if ($object) {
        $usedoaiurls = array();
        foreach ($sources as $source) {
          $oaiurl = $source['oaipmh_url'];
          if (isset($usedoaiurls[$oaiurl])) {
            break;
          }
          $usedoaiurls[$oaiurl] = 1;
          $idprefix = $source['id_prefix'];
          if (isset($idprefix)) {
            if (strlen($id) < strlen($idprefix)) {
              $id = $idprefix . $id;
            } 
          }
          $mdxsl = $source['mdxsl'];
          $mdprefix = $source['mdprefix'];
          $dsid = $source['target_dsid'];

          $t = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$oaiurl,$id,$mdprefix,$mdxsl,$dsid);
          if ($t) {
            $anId = ubl_metadata_synchronization_map_identifier($id,$mapping);
            if ($id !== $anId) {
              $t = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$oaiurl,$anId,$mdprefix,$mdxsl,$dsid);
            }
          }
          if ($t) {
            $dsid = $source['id_dsid'];
            $xpath = $source['path'];
            $anId = ubl_metadata_synchronization_get_identifier($object,$dsid,$xpath);
            if ($anId != null) {
              if ($id !== $anId) {
                $t = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$oaiurl,$anId,$mdprefix,$mdxsl,$dsid);
                if ($t) {
                  $anotherId = ubl_metadata_synchronization_map_identifier($anId,$mapping);
                  if ($anId !== $anotherId) {
                    $t = ubl_metadata_synchronization_sync_metadata_for_object_using($object,$oaiurl,$anotherId,$mdprefix,$mdxsl,$dsid);
                  }
                }
              }
            }
          }
          if ($t) {
            drush_log("Cannot synchronize object $id: $t", "warning"); 
          }
          else {
            drush_log("Successfully synchronized object $id","ok");
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

