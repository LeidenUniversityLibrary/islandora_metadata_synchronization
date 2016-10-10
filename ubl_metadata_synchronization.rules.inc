<?php
/**
 * @file
 * Rules for synchronization
 */
/**
 * Implements hook_rules_event_info().
 */
function ubl_metadata_synchronization_rules_event_info() {
  return array(
    'ubl_metadata_synchronization_object_synchronized' => array(
      'group' => t('Metadata synchronization'),
      'label' => t('Object synchronized'),
      'variables' => array(
        'object' => array(
          'type' => 'islandora_object',
          'label' => t('The synchronized object'),
          'description' => t('A Tuque object of the synchronized Fedora object, as an entity.'),
        ),
      ),
    ),
    'ubl_metadata_synchronization_selective_objects_synchronized' => array(
      'group' => t('Metadata synchronization'),
      'label' => t('Selective objects synchronized'),
      'variables' => array(
      ),
    ),
  );
}


