<?php

function ubl_metadata_synchronization_get_identifier(AbstractObject $object,$dsid,$xpath = NULL) {
  $syncid = NULL;
  if (isset($object[$dsid])) {
    $content = $object[$dsid]->content;
    if (isset($xpath) && strlen($xpath) > 0) {
      $domdoc = new DOMDocument();
      if ($domdoc->loadXML($content)) {
        $domxpath = ubl_metadata_synchronization_new_xpath($domdoc);
        $domnodelist = $domxpath->query($xpath);
        if ($domnodelist->length > 0) {
          foreach ($domnodelist as $domnode) {
            $text = $domnode->textContent;
            if (isset($text) && strlen($text) > 0) {
              $syncid = $text;
              break;
            }
          }
        }
      }
      else {
        drupal_set_message(t('Datastream !dsid contains invalid XML data', array('!dsid' => $dsid)), 'warning');
      }
    }
    else {
      $syncid = $content;
    }
  }
  return $syncid; 
}

function ubl_metadata_synchronization_new_xpath($domdoc) {
  $domxpath = new DOMXPath($domdoc);
  $domxpath->registerNamespace('mods','http://www.loc.gov/mods/v3');
  $domxpath->registerNamespace('dc','http://purl.org/dc/elements/1.1/');
  $domxpath->registerNamespace('marc','http://www.loc.gov/MARC21/slim');
  $domxpath->registerNamespace('oai','http://www.openarchives.org/OAI/2.0/');

  return $domxpath;
}
