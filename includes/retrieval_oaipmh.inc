<?php

/**
 * @file
 * Functions to retrieve metadata from a OAI-PMH compatible source.
 */


/**
 * Retrieve a record identified by syncid and mdprefix from a OAI-PMH source.
 */
function islandora_metadata_synchronization_retrieve_oaipmh_metadata($oaibaseurl, $syncid, $mdprefix) {
  if (!isset($oaibaseurl)) {
    drupal_set_message(t("Please provide a valid OAI-PMH url in settings"));
    return FALSE;
  }
  $oaigetrecurl = "$oaibaseurl?verb=GetRecord&metadataPrefix=$mdprefix&identifier=$syncid";

  $data = file_get_contents($oaigetrecurl);

  if ($data === FALSE) {
    return FALSE;
  }

  $domdoc = new DOMDocument();
  if ($domdoc->loadXML($data)) {
    $domxpath = islandora_metadata_synchronization_new_xpath($domdoc);
    $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:error');
    if ($domnodelist->length == 1) {
      return FALSE;
    }
  }  

  return $data;
}

/**
 * Test if a OAI-PMH url is valid.
 */
function islandora_metadata_synchronization_test_oai_pmh_url($oaibaseurl) {
  if (isset($oaibaseurl)) {
    $oaiidurl = $oaibaseurl . "?verb=Identify";

    $data = @file_get_contents($oaiidurl);

    if (isset($data)) {
      $pattern = '#repositoryName>([^<]+)#s';
      $preg_result = preg_match($pattern, $data, $matches);

      if ($preg_result == 1) {
        return $matches[1];
      }
    }
  }
  return FALSE;
}

/**
 * Retrieve a set of identifiers for changed metadata from OAI-PMH.
 */
function islandora_metadata_synchronization_identifiers_for_changed_metadata($oaibaseurl, $params) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/objects_identifiers');
  if (isset($oaibaseurl)) {
    $oailiurl = $oaibaseurl . "?verb=ListIdentifiers";

    if (isset($params['resumptiontoken']) && strlen($params['resumptiontoken'])) {
      $oailiurl .= '&resumptionToken=' . $params['resumptiontoken'];
    }
    else {
      $oailiurl .= "&metadataPrefix=" . (isset($params['mdprefix']) ? $params['mdprefix'] : "oai_dc");
      if (isset($params['date']) && strlen($params['date'])) {
        $oailiurl .= "&from=" . $params['date'];
      }
      if (isset($params['set']) && strlen($params['set'])) {
        $oailiurl .= "&set=" . $params['set'];
      }
    }

    $data = @file_get_contents($oailiurl);

    if (isset($data)) {
      $result = array();

      $domdoc = new DOMDocument();
      if ($domdoc->loadXML($data)) {
        $domxpath = islandora_metadata_synchronization_new_xpath($domdoc);
        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:error');
        if ($domnodelist->length == 1) {
          $result["oaierror"] = $domnodelist->item(0)->textContent;
        }
        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:responseDate');
        if ($domnodelist->length == 1) {
          $result["responsedate"] = $domnodelist->item(0)->textContent;
        }
        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:ListIdentifiers/oai:header');
        foreach ($domnodelist as $domnode) {
          $identifier = $domxpath->query('oai:identifier', $domnode);
          if (isset($identifier)) {
            $result["ids"][] = $identifier->item(0)->textContent;
          }
        }

        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:ListIdentifiers/oai:resumptionToken');
        if ($domnodelist->length == 1) {
          $next_resumption_token = $domnodelist->item(0)->textContent;
          if (strlen($next_resumption_token) > 0) {
            $result["resumptiontoken"] = $next_resumption_token;
          }
        }
      }
      return $result;
    }
  }
  return FALSE;
}
