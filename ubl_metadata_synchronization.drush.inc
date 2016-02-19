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
    ),
    'aliases' => array('smds'),
    'examples' => array(
      'drush start_metadata_synchronization --source=1 --date=last',
      'drush smds --source=2 --date=2015-12-15',
      'drush smds --source=2 --date=2015-12-15 --mapping_pattern="/oai:archimedes.leidenuniv.nl:/" --mapping_replacement="oai:disc.leidenuniv.nl:"',
    ),
  );
  return $items;
}

function drush_ubl_metadata_synchronization_start_metadata_synchronization_validate() {
  $sourceid = drush_get_option('source');

  if ($sourceid != null) {
    if (preg_match("/^\d+$/", $sourceid) != 1) {
      return drush_set_error("the option source should be a number indicating the source id as configured in islandora/tools/ubl_metadata_synchronization.");
    }
  }

  $date = drush_get_option('date');
  if (preg_match("/^(?:last|\d\d\d\d-\d\d-\d\d(?:T\d\d:\d\d:\d\dZ)?)$/",$date) != 1) {
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

  return TRUE;
}

function drush_ubl_metadata_synchronization_start_metadata_synchronization() {
  module_load_include('inc','ubl_metadata_synchronization','includes/synchronize');

  $sourceid = drush_get_option('source');
  $date = drush_get_option('date');
  $mappat = drush_get_option('mapping_pattern');
  $maprep = drush_get_option('mapping_replacement');
  $mapfile = drush_get_option('mapping_file');

  $mapping = null;
  if ($mappat != null && $maprep != null) {
    $mapping = array("_pattern" => $mappat, "_replacement" => $maprep);
  }
  else if ($mapfile != null) {
    $mapping = array();
    $content = file_get_contents($mapfile);
    $contentarray = str_getcsv($content,"\n"); 
    foreach ($contentarray as $index => $line) {
      if (strlen($line) > 0) {
        $comps = str_getcsv($line,"\t");
        if (count($comps) != 2) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber: not 2 components");
        }
        $key = $comps[0];
        $value = $comps[1];
        if (array_key_exists($key,$mapping) && $mapping[$key] !== $value) {
          $linenumber = $index + 1;
          return drush_set_error("mapping_file '$mapfile' has an error on line $linenumber: source id '$key' maps to 2 different target ids");
        }
        $mapping[$key] = $value;
      }
    }
  }

  if ($date == 'last') {
    $date = variable_get('ubl_metadata_synchronization_last_synchronization_date');
    drush_log("Synchronizing from date $date");
  }
  
  if ($sourceid == null) {
    ubl_metadata_synchronization_start_sync_from_date($date,true,$mapping);
  }
  else {
     $retrieveurl = variable_get('ubl_metadata_synchronization_source'.$sourceid.'_retrieval_url');
    ubl_metadata_synchronization_start_sync_with_url_from_date($retrieveurl,$date,true,$mapping);
  }
}

