#ifndef KIKI_COMPONENTS_MESHCOMPONENT
#define KIKI_COMPONENTS_MESHCOMPONENT

/**
 * @brief Stores the mesh ID of the object.
 * 
 * Query the MeshManager with the ID to get the actual mesh.
 * 
 * @param id the mesh ID
 */
struct MeshComponent {
    int id = -1;
};

#endif