

#define TAG_SIZE 4
#define MAX_VALID_TAGS 3

uint8_t valid_tags[MAX_VALID_TAGS][TAG_SIZE] = {
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xEA, 0xEE, 0x85, 0x6A },
    { 0x40, 0x8B, 0xE6, 0x30 }
};



/**
 * @brief Validates a tag against the list of valid tags.
 * @details This function checks if the provided tag matches any of the valid tags.
 * @param tag Pointer to the tag data to validate.
 * @param len Length of the tag data. Must be equal to TAG_SIZE.
 * @return true if the tag is valid, false otherwise.
 */
static bool security_tagValidation(uint8_t* tag, size_t len)
{
    if(tag == NULL || len != TAG_SIZE) 
    {
        ESP_LOGE(TAG, "Invalid parameters in security_tagValidation");
        return false;
    }

    for(int i = 0; i < MAX_VALID_TAGS; i++) 
        if(memcmp(tag, valid_tags[i], TAG_SIZE) == 0) 
            return true;

    return false;
}