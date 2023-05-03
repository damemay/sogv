#include <sogv.h>

static void sogv_vec3_lerp(vec3 from, vec3 to, float t, vec3 dest) {
    vec3 s, v;
    s[0] = s[1] = s[2] = t;
    vec3_sub(v, to, from);
    v[0] = s[0]*v[0];
    v[1] = s[1]*v[1];
    v[2] = s[2]*v[2];
    vec3_add(dest, from, v);
}

static void sogv_vec4_lerp(vec4 from, vec4 to, float t, vec4 dest) {
    vec4 s, v;
    s[0] = s[1] = s[2] = s[3] = t;
    vec4_sub(v, to, from);
    v[0] = s[0]*v[0];
    v[1] = s[1]*v[1];
    v[2] = s[2]*v[2];
    v[3] = s[3]*v[3];
    vec4_add(dest, from, v);
}

static void sogv_vec4_copy(vec4 from, vec4 to) {
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
    to[3] = from[3];
}

static void sogv_quat_slerp(quat from, quat to, float t, quat dest) {
    vec4 q1, q2;
    float cos_theta, sin_theta, angle;

    cos_theta = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
    sogv_vec4_copy(from, q1);

    if(fabsf(cos_theta) >= 1.0f) {
        sogv_vec4_copy(q1, dest);
        return;
    }
    if(cos_theta < 0.0f) {
        q1[0] = -q1[0];
        q1[1] = -q1[1];
        q1[2] = -q1[2];
        q1[3] = -q1[3];
        cos_theta = -cos_theta;
    }

    sin_theta = sqrtf(1.0f - cos_theta * cos_theta);

    if(fabsf(sin_theta) < 0.001f) {
        sogv_vec4_lerp(from, to, t, dest);
        return;
    }

    angle = acosf(cos_theta);
    vec4_scale(q1, q1, sinf((1.0f-t)*angle));
    vec4_scale(q2, to, sinf(t*angle));
    vec4_add(q1, q1, q2);
    vec4_scale(dest, q1, 1.0f/sin_theta);
}

static void sogv_assimp_vec3(vec3 vec, struct aiVector3D ai_vec) {
    vec[0] = ai_vec.x;
    vec[1] = ai_vec.y;
    vec[2] = ai_vec.z;
}

static void sogv_assimp_vec2(vec2 vec, struct aiVector3D ai_vec) {
    vec[0] = ai_vec.x;
    vec[1] = ai_vec.y;
}

static void sogv_assimp_quat(quat q, struct aiQuaternion ai_q) {
    q[0] = ai_q.x;
    q[1] = ai_q.y;
    q[2] = ai_q.z;
    q[3] = ai_q.w;
}

static void sogv_assimp_mat4x4(mat4x4 mat, struct aiMatrix4x4 ai_mat) {
    mat[0][0] = ai_mat.a1;
    mat[0][1] = ai_mat.b1;
    mat[0][2] = ai_mat.c1;
    mat[0][3] = ai_mat.d1;

    mat[1][0] = ai_mat.a2;
    mat[1][1] = ai_mat.b2;
    mat[1][2] = ai_mat.c2;
    mat[1][3] = ai_mat.d2;

    mat[2][0] = ai_mat.a3;
    mat[2][1] = ai_mat.b3;
    mat[2][2] = ai_mat.c3;
    mat[2][3] = ai_mat.d3;

    mat[3][0] = ai_mat.a4;
    mat[3][1] = ai_mat.b4;
    mat[3][2] = ai_mat.c4;
    mat[3][3] = ai_mat.d4;
}

static void sogv_mesh_glize(sogv_mesh* mesh) {
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->vert_count * sizeof(sogv_vert), mesh->verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indice_count * sizeof(uint), mesh->indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(SOGV_ATTR_POSITION_ID);
    glVertexAttribPointer(SOGV_ATTR_POSITION_ID, 3, GL_FLOAT, GL_FALSE,
            sizeof(sogv_vert), (void*)0);

    glEnableVertexAttribArray(SOGV_ATTR_NORMAL_ID);
    glVertexAttribPointer(SOGV_ATTR_NORMAL_ID, 3, GL_FLOAT, GL_FALSE,
            sizeof(sogv_vert), (void*)offsetof(sogv_vert, normal));

    glEnableVertexAttribArray(SOGV_ATTR_UV_ID);
    glVertexAttribPointer(SOGV_ATTR_UV_ID, 2, GL_FLOAT, GL_FALSE,
            sizeof(sogv_vert), (void*)offsetof(sogv_vert, uv));

    glEnableVertexAttribArray(SOGV_ATTR_BONE_ID);
    glVertexAttribPointer(SOGV_ATTR_BONE_ID, 4, GL_FLOAT, GL_FALSE,
            sizeof(sogv_vert), (void*)offsetof(sogv_vert, bone_info));

    glEnableVertexAttribArray(SOGV_ATTR_WEIGHT_ID);
    glVertexAttribPointer(SOGV_ATTR_WEIGHT_ID, 4, GL_FLOAT, GL_FALSE,
            sizeof(sogv_vert), (void*)offsetof(sogv_vert, weights));

    glBindVertexArray(0);
}

static void sogv_mesh_render(sogv_mesh* mesh) {
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->indice_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

static void sogv_mesh_clean(sogv_mesh* mesh) {
    free(mesh->verts);
    free(mesh->indices);
    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
}

static const struct aiScene* sogv_assimp_scene_load(const char* path) {
    const struct aiScene* scene = aiImportFile(path, aiProcess_JoinIdenticalVertices |
                                                aiProcess_Triangulate | aiProcess_FlipUVs);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
        sogv_die_v("Could not load assimp scene from %s", path);

    return scene;
}

static int sogv_skel_node_import(const struct aiNode* ai_node, sogv_skel_node** skel_node,
                        size_t bone_count, char bone_names[][64]) {
    sogv_skel_node* t_node = calloc(1, sizeof(sogv_skel_node));
    t_node->bone_idx = -1;
    t_node->child_count = 0;
    t_node->pos_keys = NULL;
    t_node->rot_keys = NULL;
    t_node->sca_keys = NULL;
    t_node->pos_keys_count = 0;
    t_node->rot_keys_count = 0;
    t_node->sca_keys_count = 0;
    t_node->pos_key_times = NULL;
    t_node->rot_key_times = NULL;
    t_node->sca_key_times = NULL;
    strncpy(t_node->name, ai_node->mName.data, 63);
    sogv_log_v("node name: %s", t_node->name);
    sogv_log_v("ai_node has %u children", ai_node->mNumChildren);
    
    for(size_t i=0; i<MAX_BONES; ++i)
        t_node->children[i] = NULL;

    bool has_bone = false;
    for(size_t i=0; i<bone_count; ++i)
        if(strcmp(bone_names[i], t_node->name)==0) {
            sogv_log_v("node will use bone %zu : %s", i, t_node->name);
            t_node->bone_idx = i;
            has_bone = true;
            break;
        }
    if(!has_bone) sogv_log("no bones found");

    bool has_usable_child = false;
    for(size_t i=0; i<ai_node->mNumChildren; ++i) {
        if(sogv_skel_node_import(ai_node->mChildren[i],
                    &t_node->children[t_node->child_count], bone_count, bone_names)==0) {
            has_usable_child = true;
            t_node->child_count++;
        } else sogv_log("non-usable child node thrown away");
    }
    if(has_usable_child || has_bone) {
        *skel_node = t_node;
        return 0;
    }

    free(t_node);
    t_node = NULL;
    return 1;
}

static sogv_skel_node* sogv_skel_node_find(sogv_skel_node* root, const char* name) {
    if(strcmp(name, root->name)==0)
        return root;

    for(size_t i=0; i<root->child_count; ++i) {
        sogv_skel_node* child = sogv_skel_node_find(root->children[i], name);
        if(child) return child;
    }

    return NULL;
}

static void sogv_skel_node_clean(sogv_skel_node* node) {
    if(node->pos_keys) free(node->pos_keys);
    if(node->rot_keys) free(node->rot_keys);
    if(node->sca_keys) free(node->sca_keys);
    if(node->pos_key_times) free(node->pos_key_times);
    if(node->rot_key_times) free(node->rot_key_times);
    if(node->sca_key_times) free(node->sca_key_times);
    for(size_t i=0; i<node->child_count; ++i)
        sogv_skel_node_clean(node->children[i]);
}

sogv_model* sogv_model_create(const char* folder, const char* file) {
    char* model_path = calloc(strlen(folder)+strlen(file)+1, sizeof(char));
    strcpy(model_path, folder);
    strcat(model_path, file);
    const struct aiScene* scene = sogv_assimp_scene_load(model_path);
    if(!scene) sogv_die("Could not load assimp scene");
    free(model_path);

    size_t ai_mesh_count = scene->mNumMeshes;
    size_t ai_mat_count = scene->mNumMaterials;

    // Setup the model
    sogv_model* _model = calloc(1, sizeof(sogv_model));
    _model->meshes = calloc(ai_mesh_count, sizeof(sogv_mesh));
    _model->materials =  calloc(ai_mat_count, sizeof(GLuint));
    _model->mesh_count = ai_mesh_count;
    _model->mat_count = ai_mat_count;
    _model->bone_count = 0;
    _model->root_node = NULL;

    // Setup the mesh
    for(size_t mesh_idx = 0; mesh_idx < ai_mesh_count; ++mesh_idx) {
        const struct aiMesh* ai_mesh = scene->mMeshes[mesh_idx];
        const size_t ai_vert_count = ai_mesh->mNumVertices;
        const size_t ai_face_count = ai_mesh->mNumFaces;
        const size_t ai_bone_count = ai_mesh->mNumBones;
        size_t ai_indice_count = 0;

        sogv_mesh _mesh = {
            .verts = calloc(ai_vert_count, sizeof(sogv_vert)),
            .indices = NULL,
            .vert_count = ai_vert_count,
            .indice_count = 0,
            .mat_idx = ai_mesh->mMaterialIndex
        };

        // Copy vert data
        for(size_t i = 0; i < ai_vert_count; ++i) {
            sogv_assimp_vec3(_mesh.verts[i].pos, ai_mesh->mVertices[i]);
            if(ai_mesh->mNormals)
                sogv_assimp_vec3(_mesh.verts[i].normal, ai_mesh->mNormals[i]);
            if(ai_mesh->mTextureCoords[0])
                sogv_assimp_vec2(_mesh.verts[i].uv, ai_mesh->mTextureCoords[0][i]);
        }

        // Count indices
        for(size_t i=0; i<ai_face_count; ++i)
            ai_indice_count += ai_mesh->mFaces[i].mNumIndices;

        // Now set them up and copy
        _mesh.indice_count = ai_indice_count;
        _mesh.indices = calloc(ai_indice_count, sizeof(uint));

        {
            size_t iter = 0;
            for(size_t i=0; i<ai_face_count; ++i)
                for(size_t j=0; j<ai_mesh->mFaces[i].mNumIndices; ++j)
                    _mesh.indices[iter++] = ai_mesh->mFaces[i].mIndices[j];
        }

        // Setup bones to a model
        if(ai_bone_count>0) {
            char bone_names[MAX_BONES][64];
            // go throught the ai_bone_count and see if we already have the same name inside our model
            // if we dont have the name: add; if we have - skip.
            for(size_t i=0; i<ai_bone_count; ++i) {
                const struct aiBone* ai_bone = ai_mesh->mBones[i];
                strncpy(bone_names[i], ai_bone->mName.data, 63);

                bool exists = false;
                for(size_t j=0; j<MAX_BONES; ++j)
                    if(strcmp(bone_names[i], _model->bone_names[j])==0) {
                        exists = true;
                        sogv_log_v("bone %s is already saved", bone_names[i]);
                }

                if(!exists) {
                    sogv_assimp_mat4x4(_model->bones[i], ai_bone->mOffsetMatrix);
                    strncpy(_model->bone_names[i], bone_names[i], 63);
                    _model->bone_count++;
                }

                // Setup bone weights
                const size_t ai_weight_count = ai_bone->mNumWeights;
                for(size_t j=0; j<ai_weight_count; ++j) {
                    struct aiVertexWeight ai_weight = ai_bone->mWeights[j];
                    uint v_i = ai_weight.mVertexId;
                    for(size_t k=0; k<MAX_BONE_INFLUENCE; ++k) {
                        if(_mesh.verts[v_i].weights[k]==0.0f) {
                            _mesh.verts[v_i].bone_info[k] = i;
                            _mesh.verts[v_i].weights[k] = ai_weight.mWeight;
                            break;
                        }
                    }
                }
            }
        }

        // Copy everything to GL buffers and put into model array

        sogv_mesh_glize(&_mesh);
        _model->meshes[mesh_idx] = _mesh;
    }
    sogv_log_v("Model bone count: %zu", _model->bone_count);
    for(size_t i=0; i<_model->bone_count; ++i) sogv_log_v("bone %zu : %s", i, _model->bone_names[i]);

    // Setup skeleton nodes
    const struct aiNode* ai_node = scene->mRootNode;
    if(sogv_skel_node_import(ai_node, &_model->root_node, _model->bone_count, _model->bone_names)==1)
        sogv_log("No skeleton found inside the model");

    // Setup first animation
    if(scene->mNumAnimations > 0) {
        const struct aiAnimation* anim = scene->mAnimations[0];
        sogv_log_v("animation has a name: %s", anim->mName.data);
        sogv_log_v("animation has %u nodechannels", anim->mNumChannels);
        sogv_log_v("animation has %u meshchannels", anim->mNumMeshChannels);
        sogv_log_v("animation is %f long", anim->mDuration);
        sogv_log_v("animation has %f ticks per second", anim->mTicksPerSecond);
        _model->anim_dur = anim->mDuration;
        _model->anim_ticks = anim->mTicksPerSecond;

        for(size_t i=0; i<anim->mNumChannels; ++i) {
            const struct aiNodeAnim* channel = anim->mChannels[i];
            sogv_skel_node* node = sogv_skel_node_find(_model->root_node, channel->mNodeName.data);
            node->pos_keys_count = channel->mNumPositionKeys;
            node->rot_keys_count = channel->mNumRotationKeys;
            node->sca_keys_count = channel->mNumScalingKeys;
            node->pos_keys = calloc(node->pos_keys_count, sizeof(vec3));
            node->rot_keys = calloc(node->rot_keys_count, sizeof(quat));
            node->sca_keys = calloc(node->sca_keys_count, sizeof(vec3));
            node->pos_key_times = calloc(node->pos_keys_count, sizeof(float));
            node->rot_key_times = calloc(node->rot_keys_count, sizeof(float));
            node->sca_key_times = calloc(node->sca_keys_count, sizeof(float));

            for(size_t j=0; j<node->pos_keys_count; ++j) {
                sogv_assimp_vec3(node->pos_keys[j], channel->mPositionKeys[j].mValue);
                node->pos_key_times[j] = channel->mPositionKeys[j].mTime;
            }

            for(size_t j=0; j<node->rot_keys_count; ++j) {
                sogv_assimp_quat(node->rot_keys[j], channel->mRotationKeys[j].mValue);
                node->rot_key_times[j] = channel->mRotationKeys[j].mTime;
            }

            for(size_t j=0; j<node->sca_keys_count; ++j) {
                sogv_assimp_vec3(node->sca_keys[j], channel->mScalingKeys[j].mValue);
                node->sca_key_times[j] = channel->mScalingKeys[j].mTime;
            }
        }
    }

    for(size_t m_idx = 0; m_idx < ai_mat_count; ++m_idx) {
        struct aiString ai_str;
        if(aiGetMaterialTexture(scene->mMaterials[m_idx], aiTextureType_DIFFUSE, 0, &ai_str,
                    NULL, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            char* tex_path = calloc(strlen(folder)+strlen(ai_str.data)+1, sizeof(char));
            strcpy(tex_path, folder);
            strcat(tex_path, ai_str.data);
            _model->materials[m_idx] = sogv_gl_stb_texture_create(tex_path);
            free(tex_path);
        }
    }

    aiReleaseImport(scene);
    return _model;
}

void sogv_model_render(sogv_model* model) {
    for(size_t i=0; i<model->mesh_count; ++i) {
        glBindTexture(GL_TEXTURE_2D, model->materials[model->meshes[i].mat_idx]);
        sogv_mesh_render(&model->meshes[i]);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void sogv_model_free(sogv_model* model) {
    for(size_t i=0; i<model->mesh_count; ++i)
        sogv_mesh_clean(&model->meshes[i]);
    free(model->meshes);
    free(model->materials);
    if(model->root_node) sogv_skel_node_clean(model->root_node);

    free(model);
}

void sogv_skel_animate(sogv_skel_node* node, float anim_time, mat4x4 parent_mat, mat4x4* bones,
        mat4x4* bone_anim_mats) {
    // our -> base animation; local -> matrix that moves;
    mat4x4 our_mat, local_anim_mat;
    mat4x4_dup(our_mat, parent_mat);
    mat4x4_identity(local_anim_mat);

    mat4x4 t_node;
    mat4x4_identity(t_node);
    if(node->pos_keys_count>0) {
        size_t p_key = 0;
        size_t n_key = 0;
        for(size_t i=0; i<node->pos_keys_count-1; ++i) {
            p_key = i;
            n_key = i+1;
            if(node->pos_key_times[n_key]>=anim_time) break;
        }
        float t_tot = node->pos_key_times[n_key] - node->pos_key_times[p_key];
        float t = (anim_time - node->pos_key_times[p_key]) / t_tot;
        vec3 vi, vf;
        vec3_dup(vi, node->pos_keys[p_key]);
        vec3_dup(vf, node->pos_keys[n_key]);
        vec3 lerp0, lerp1, lerp;
        vec3_scale(lerp0, vi, 1.0f-t);
        vec3_scale(lerp1, vf, t);
        vec3_add(lerp, lerp0, lerp1);
        mat4x4_translate_in_place(t_node, lerp[0], lerp[1], lerp[2]);
    }

    /*mat4x4 s_node;
    mat4x4_identity(s_node);
    if(node->sca_keys_count>0) {
        size_t p_key = 0;
        size_t n_key = 0;
        for(size_t i=0; i<node->sca_keys_count-1; ++i) {
            p_key = i;
            n_key = i+1;
            if(node->sca_key_times[n_key]>=anim_time) break;
        }
        float t_tot = node->sca_key_times[n_key] - node->sca_key_times[p_key];
        float t = (anim_time - node->sca_key_times[p_key]) / t_tot;
        vec3 vi, vf;
        vec3_dup(vi, node->sca_keys[p_key]);
        vec3_dup(vf, node->sca_keys[n_key]);
        vec3 lerp0, lerp1, lerp;
        vec3_scale(lerp0, vi, 1.0f-t);
        vec3_scale(lerp1, vf, t);
        vec3_add(lerp, lerp0, lerp1);
        mat4x4_translate_in_place(t_node, lerp[0], lerp[1], lerp[2]);
    }*/

    mat4x4 r_node;
    mat4x4_identity(r_node);
    if(node->rot_keys_count>0) {
        size_t p_key = 0;
        size_t n_key = 0;
        for(size_t i=0; i<node->rot_keys_count-1; ++i) {
            p_key = i;
            n_key = i+1;
            if(node->rot_key_times[n_key]>=anim_time) break;
        }
        float t_tot = node->rot_key_times[n_key] - node->rot_key_times[p_key];
        float t = (anim_time - node->rot_key_times[p_key]) / t_tot;
        quat vi, vf;
        vec4_dup(vi, node->rot_keys[p_key]);
        vec4_dup(vf, node->rot_keys[n_key]);
        quat lerp;
        sogv_quat_slerp(vi, vf, t, lerp);
        mat4x4_from_quat(r_node, lerp);
    }

    mat4x4_mul(local_anim_mat, t_node, r_node);
    // mat4x4_mul(local_anim_mat, local_anim_mat, s_node);

    int bone_i = node->bone_idx;
    if(bone_i > -1) {
        mat4x4 bone_offset;
        mat4x4_dup(bone_offset, bones[bone_i]);
        mat4x4_mul(our_mat, parent_mat, local_anim_mat);
        mat4x4 whatsthis;
        mat4x4_mul(whatsthis, our_mat, bone_offset);
        mat4x4_dup(bone_anim_mats[bone_i], whatsthis);
    }
    for(size_t i=0; i<node->child_count; ++i)
        sogv_skel_animate(node->children[i], anim_time, our_mat, bones, bone_anim_mats);
}

sogv_cam sogv_cam_create(const float pos_x, const float pos_y, const float pos_z,
                                const float mov_spd, const float rot_spd) {
    sogv_cam new = {
        .position = {pos_x, pos_y, pos_z},
        .front = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .yaw = -90.0f,
        .pitch = 0.0f,
        .mov_spd = mov_spd,
        .rot_spd = rot_spd,
        .moving = false, .rotating = false
    };

    return new;
};

void sogv_cam_handle_events(sogv_cam* cam, const SDL_Event e) {
    if(e.type == SDL_KEYDOWN && e.key.repeat == 0) {
        switch(e.key.keysym.sym) {
            case SDLK_w: cam->moving = true; cam->mov_dir = FRONT; break;
            case SDLK_s: cam->moving = true; cam->mov_dir = BACK; break;
            case SDLK_a: cam->moving = true; cam->mov_dir = LEFT; break;
            case SDLK_d: cam->moving = true; cam->mov_dir = RIGHT; break;

            case SDLK_UP:    cam->rotating = true; cam->rot_dir = FRONT; break;
            case SDLK_DOWN:  cam->rotating = true; cam->rot_dir = BACK; break;
            case SDLK_LEFT:  cam->rotating = true; cam->rot_dir = LEFT; break;
            case SDLK_RIGHT: cam->rotating = true; cam->rot_dir = RIGHT; break;
        }
    } else if(e.type == SDL_KEYUP && e.key.repeat == 0) {
        switch(e.key.keysym.sym) {
            case SDLK_w: case SDLK_s:
            case SDLK_a: case SDLK_d:
                cam->moving = false; break;

            case SDLK_UP: case SDLK_DOWN:
            case SDLK_LEFT: case SDLK_RIGHT:
                cam->rotating = false; break;
        }
    } else if(e.type == SDL_CONTROLLERBUTTONDOWN) {
        switch(e.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:    cam->moving = true; cam->mov_dir = FRONT; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  cam->moving = true; cam->mov_dir = BACK; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  cam->moving = true; cam->mov_dir = LEFT; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: cam->moving = true; cam->mov_dir = RIGHT; break;
        }
    } else if(e.type == SDL_CONTROLLERBUTTONUP) {
        switch(e.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP: case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                cam->moving = false; break;
        }
    } else if(e.type == SDL_CONTROLLERAXISMOTION) {
        switch(e.caxis.axis) {
            case SDL_CONTROLLER_AXIS_RIGHTX:
                if(e.caxis.value > ANALOG_DEADZONE)       { cam->rotating = true; cam->rot_dir = RIGHT; }
                else if(e.caxis.value < -ANALOG_DEADZONE) { cam->rotating = true; cam->rot_dir = LEFT; }
                else cam->rotating = false; break;
        }
    }
}

void sogv_cam_movement(sogv_cam* cam, const float ticks) {
    vec3 front, cam_vel, c, n;
    if(cam->rotating) switch(cam->rot_dir) {
        case FRONT: break;
        case BACK: break;
        case LEFT:
            cam->yaw -= cam->rot_spd * ticks;
            front[0] = cos(sogv_deg_to_rad(cam->yaw));
            front[1] = 0.0f;
            front[2] = sin(sogv_deg_to_rad(cam->yaw));
            vec3_norm(cam->front, front);
            break;
        case RIGHT:
            cam->yaw += cam->rot_spd * ticks;
            front[0] = cos(sogv_deg_to_rad(cam->yaw));
            front[1] = 0.0f;
            front[2] = sin(sogv_deg_to_rad(cam->yaw));
            vec3_norm(cam->front, front);
            break;
    }

    if(cam->moving) switch(cam->mov_dir) {
        case FRONT:
            vec3_scale(cam_vel, cam->front, cam->mov_spd * ticks);
            vec3_add(cam->position, cam->position, cam_vel);
            break;
        case BACK:
            vec3_scale(cam_vel, cam->front, cam->mov_spd * ticks);
            vec3_sub(cam->position, cam->position, cam_vel);
            break;
        case LEFT:
            vec3_mul_cross(c, cam->front, cam->up);
            vec3_norm(n, c);
            vec3_scale(cam_vel, n, cam->mov_spd * ticks);
            vec3_sub(cam->position, cam->position, cam_vel);
            break;
        case RIGHT:
            vec3_mul_cross(c, cam->front, cam->up);
            vec3_norm(n, c);
            vec3_scale(cam_vel, n, cam->mov_spd * ticks);
            vec3_add(cam->position, cam->position, cam_vel);
            break;
    }
}
