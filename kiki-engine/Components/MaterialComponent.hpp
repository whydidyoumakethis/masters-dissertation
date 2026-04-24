#ifndef KIKI_COMPONENTS_MATERIALCOMPONENT
#define KIKI_COMPONENTS_MATERIALCOMPONENT

/**
 * @brief Stores the material ID of the object.
 * 
 * Query the SceneManager with the ID to get the actual material.
 * 
 * @param id the material ID
 */
struct MaterialComponent {
    int id = -1;
};

#endif