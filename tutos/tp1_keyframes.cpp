
//! \file tuto7.cpp reprise de tuto6.cpp mais en derivant App::init(), App::quit() et bien sur App::render().

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"
#include "program.h"
#include "uniforms.h"
#include "app_time.h"        // classe Application a deriver

namespace glsl {
    struct alignas(16) vec4
    {
        float x, y, z, w;
        vec4() {}
        vec4(const ::vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
        vec4(const Color& v) : x(v.r), y(v.g), z(v.b), w(v.a) {}
    };
    struct alignas(16) vec3
    {
        float x, y, z;
        vec3() {}
        vec3(const ::vec3& v) : x(v.x), y(v.y), z(v.z) {}
        vec3(const Color& v) : x(v.r), y(v.g), z(v.b) {}
    };
    struct alignas(16) gfloat{
        float f;
        gfloat() {}
        gfloat(const float& v) : f(v) {}
    };

}

struct materialData {
    glsl::vec4 ambient;
    glsl::vec4 diffuse;
    glsl::vec4 specular;
    glsl::gfloat ns;

    materialData() : ambient(), diffuse(), specular(), ns() {}
};


struct Buffers
{
    GLuint vao;
    GLuint buffer;
    GLuint *buffers;
    GLuint materials_buffer;
    int count;
    GLuint framebuffer;

    Buffers( ) : vao(0),buffer(0), materials_buffer(0), count(0) {}

    void create( const Mesh& mesh )
    {
        if(!mesh.vertex_buffer_size()) return;

        // cree et configure le vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // cree et initialise le buffer: conserve la positions des sommets
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        size_t size = mesh.vertex_buffer_size() + mesh.texcoord_buffer_size();
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

        //transfere les positions des sommets
        size_t offset = 0;
        size = mesh.vertex_buffer_size();

        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        glVertexAttribPointer(0,
            3, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            nullptr               // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        glBufferSubData(GL_ARRAY_BUFFER, offset,  mesh.vertex_buffer_size(), mesh.vertex_buffer());

        offset = offset + size;
        //offset = size;
        size= mesh.texcoord_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, /* stride */ 0, nullptr);
        glEnableVertexAttribArray(1);

        glBufferSubData(GL_ARRAY_BUFFER, offset, mesh.texcoord_buffer_size(), mesh.texcoord_buffer());



        // conserve le nombre de sommets
        count= mesh.vertex_count();
    }

    void create2( const Mesh& mesh )
    {
        if(!mesh.vertex_buffer_size()) return;

        // cree et configure le vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // cree et initialise le buffer: conserve la positions des sommets
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        size_t size = mesh.vertex_buffer_size() + mesh.normal_buffer_size();
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

        //transfere les positions des sommets
        size_t offset = 0;
        size = mesh.vertex_buffer_size();



        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        glVertexAttribPointer(0,
            3, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            nullptr               // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        glBufferSubData(GL_ARRAY_BUFFER, offset,  mesh.vertex_buffer_size(), mesh.vertex_buffer());

        offset = offset + size;
        size= mesh.normal_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *)offset);
        glEnableVertexAttribArray(1);

        glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh.normal_buffer());

        std::vector<materialData> data(mesh.triangle_count());
        std::vector<unsigned int> mats = mesh.materials();
        for (size_t i=0; i < mesh.triangle_count(); i++) {
            data[i].ambient = glsl::vec4(mesh.mesh_material(mats[i]).emission);
            data[i].diffuse = glsl::vec4(mesh.mesh_material(mats[i]).diffuse);
            data[i].specular = glsl::vec4(mesh.mesh_material(mats[i]).specular);
            data[i].ns = glsl::gfloat(mesh.mesh_material(mats[i]).ns);
        }

        glGenBuffers(1, &materials_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials_buffer);

        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(materialData) * data.size(), data.data(), GL_STREAM_READ);

        // conserve le nombre de sommets
        count= mesh.vertex_count();
    }



    void create2keyframes( const Mesh& m1, const Mesh& m2)
    {
        if(!m1.vertex_buffer_size()) return;

        if(m1.vertex_buffer_size() != m2.vertex_buffer_size()) return;

        // cree et configure le vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // cree et initialise le buffer: conserve la positions des sommets
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        size_t size = m1.vertex_buffer_size() + m2.vertex_buffer_size() + m1.normal_buffer_size() + m2.normal_buffer_size();
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

        //transfere les positions des sommets
        size_t offset = 0;
        size = m1.vertex_buffer_size();

        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        glVertexAttribPointer(0,
            3, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            (const GLvoid *)offset               // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        glBufferSubData(GL_ARRAY_BUFFER, offset,  m1.vertex_buffer_size(), m1.vertex_buffer());

        offset = offset + size;
        //offset = size;
        size= m1.normal_buffer_size();

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(1);

        glBufferSubData(GL_ARRAY_BUFFER, offset,size, m1.normal_buffer());


        offset = offset + size;
        //offset = size;
        size= m2.vertex_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(2);

        glBufferSubData(GL_ARRAY_BUFFER, offset,size, m2.vertex_buffer());


        offset = offset + size;
        //offset = size;
        size= m2.normal_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(3);

        glBufferSubData(GL_ARRAY_BUFFER, offset,size, m2.normal_buffer());
        // conserve le nombre de sommets
        count= m1.vertex_count();

        std::vector<materialData> data(m1.triangle_count());
        std::vector<unsigned int> mats = m1.materials();
        for (size_t i=0; i < m1.triangle_count(); i++) {
            data[i].ambient = glsl::vec4(m1.mesh_material(mats[i]).emission);
            data[i].diffuse = glsl::vec4(m1.mesh_material(mats[i]).diffuse);
            data[i].specular = glsl::vec4(m1.mesh_material(mats[i]).specular);
            data[i].ns = glsl::gfloat(m1.mesh_material(mats[i]).ns);
        }

        glGenBuffers(1, &materials_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials_buffer);



        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(materialData) * data.size(), data.data(), GL_STREAM_READ);


    }

    void createkeyframes(std::vector<Mesh> m){
        if(!m[0].vertex_buffer_size()) return;

        if(m[0].vertex_buffer_size() != m[1].vertex_buffer_size()) return;

        buffers = new GLuint[m.size()];

        // cree et configure le vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // cree et initialise le buffer: conserve la positions des sommets
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        size_t size = m[0].vertex_buffer_size() * m.size() + m[0].normal_buffer_size() * m.size();
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

        //transfere les positions des sommets
        size_t offset = 0;
        size = m[0].vertex_buffer_size();

        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        glVertexAttribPointer(0,3, GL_FLOAT,GL_FALSE,0,(const GLvoid *)offset);
        glEnableVertexAttribArray(0);

        offset = offset + size;
        size= m[0].normal_buffer_size();

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(1);

        offset = offset + size;
        size= m[1].vertex_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(2);

        offset = offset + size;
        size= m[1].normal_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(3);


        size_t offsetIt = 0;
        size_t sizeIt = 0;

        for (auto it = m.begin(); it!=m.end(); it++) {
            offsetIt += sizeIt;
            sizeIt = (*it).vertex_buffer_size();
            glBufferSubData(GL_ARRAY_BUFFER, offsetIt,  sizeIt, (*it).vertex_buffer());
            offsetIt += sizeIt;
            sizeIt = (*it).normal_buffer_size();
            glBufferSubData(GL_ARRAY_BUFFER, offsetIt,  sizeIt, (*it).normal_buffer());

        }



        std::vector<materialData> data(m[0].triangle_count());
        std::vector<unsigned int> mats = m[0].materials();
        for (size_t i=0; i < m[0].triangle_count(); i++) {
            data[i].ambient = glsl::vec4(m[0].mesh_material(mats[i]).emission);
            data[i].diffuse = glsl::vec4(m[0].mesh_material(mats[i]).diffuse);
            data[i].specular = glsl::vec4(m[0].mesh_material(mats[i]).specular);
            data[i].ns = glsl::gfloat(m[0].mesh_material(mats[i]).ns);
        }

        glGenBuffers(1, &materials_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materials_buffer);

        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(materialData) * data.size(), data.data(), GL_STREAM_READ);

        // conserve le nombre de sommets
        count= m[0].vertex_count();
    }

    void genFrameBuffer() {

    }



    void release( )
    {
        glDeleteBuffers(1, &buffer);
        glDeleteVertexArrays(1, &vao);
    }
};



class TP : public AppTime
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppTime(1024, 640) {}

    // creation des objets de l'application
    int init( )
    {
        m_meshes.push_back(read_mesh("data/run/Robot_000001.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000002.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000003.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000004.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000005.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000006.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000007.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000008.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000009.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000010.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000011.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000012.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000013.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000014.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000015.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000016.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000017.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000018.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000019.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000020.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000021.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000022.obj"));
        m_meshes.push_back(read_mesh("data/run/Robot_000023.obj"));

        m_objet.createkeyframes(m_meshes);

        m_floor = read_mesh("data/floor30x30.obj");
        m_objet2.create2(m_floor);
        std::for_each(m_floor.normals().begin(), m_floor.normals().end(), [](const vec3 e){std::cout << "Normale :" << e.x << " " << e.y << " " << e.z << std::endl;});

        Point pmin, pmax, pmin2, pmax2;
        m_meshes[0].bounds(pmin, pmax);
        m_floor.bounds(pmin2, pmax2);
        m_camera.lookat(min(pmin, pmin2), max(pmax, pmax2));

        frames.push_back(0);
        t0 = global_time()/(1000.f/24.f);
        t1 = t0 + 1000/24.f;
        dt = 0;

        m_program = read_program("tutos/tp1_keyframes.glsl");
        program_print_errors(m_program);

        m_program2 = read_program("tutos/tp1_static.glsl");
        program_print_errors(m_program2);

        m_program_shadow = read_program("tutos/tp1_keyframes_shadow.glsl");
        program_print_errors(m_program_shadow);

        m_program_shadow2 = read_program("tutos/tp1_static_shadow.glsl");
        program_print_errors(m_program_shadow2);

        glGenTextures(1, &shadow_map);
        glBindTexture(GL_TEXTURE_2D, shadow_map);

        // configuration framebuffer / texture
        glTexImage2D(GL_TEXTURE_2D,
            /* level */ 0,
            /* texel format */ GL_DEPTH_COMPONENT,
            /* width, height, border */5 * window_width(), 5 * window_height(), 0,
            /* data format */ GL_DEPTH_COMPONENT, /* data type */ GL_UNSIGNED_INT,
            /* data */ nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        //glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_COLOR_ATTACHMENT0, /* texture */ color_texture, /* mipmap level */ 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_DEPTH_ATTACHMENT, /* texture */ shadow_map, /* mipmap level */ 0);
        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre

        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);    // activer le ztest


        return 0;   // ras, pas d'erreur
    }

    // destruction des objets de l'application
    int quit( )
    {
        m_objet.release();
        m_objet2.release();
        release_program(m_program);
        release_program(m_program2);
        release_program(m_program_shadow);
        release_program(m_program_shadow2);
        // nettoyage
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDeleteTextures(1, &shadow_map);
        glDeleteTextures(1, &color_texture);

        return 0;
    }

    // dessiner une nouvelle image
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // deplace la camera

        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))              // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if(mb & SDL_BUTTON(3))         // le bouton droit est enfonce
            m_camera.move(mx);
        else if(mb & SDL_BUTTON(2))         // le bouton du milieu est enfonce
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height());
        else if (key_state('a')){
            clear_key_state('a');
            frames.push_back(0);
            Point pmin, pmax, pmin2, pmax2;
            m_meshes[0].bounds(pmin, pmax);
            m_floor.bounds(pmin2, pmax2);
            m_camera.lookat(min(pmin, pmin2), max(Translation((frames.size() - 1) * 20, 0 ,0)(pmax), pmax2));
        }
        if(dt >= 1) {
            std::for_each(frames.begin(), frames.end(), [](int &n){ n = (n+1)%22; });
            t0 = global_time()/(1000.f/24.f);
            t1 = t0 + 1000/24.f;
        }

        Transform ortho = Ortho(-30.0f,30.0f,-30.0f,30.0f,0.0001f,1000.0f);
        Transform orthoView = Lookat(Point(50,250,0), Origin(), Vector(0,0,1));
        Transform invOrthoView = Inverse(orthoView);

        Transform view = m_camera.view();
        Transform projection = m_camera.projection(window_width(), window_height(), 45);

        dt = (global_time()/(1000.f/24.f) - t0 )/(t1 - t0);

        Transform invView = Inverse(view);


        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

        glViewport(0,0, window_width() * 5, window_height() * 5);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program_shadow);
        glBindVertexArray(m_objet.vao);
        int location;

        location= glGetUniformLocation(m_program_shadow, "dt");
        glUniform1f(location, dt);
        location= glGetUniformLocation(m_program_shadow, "viewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, orthoView.buffer());
        location= glGetUniformLocation(m_program_shadow, "invViewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, invOrthoView.buffer());
        location= glGetUniformLocation(m_program_shadow, "lightPos");
        glUniform4f(location,50.f,250.f,0.f,1.f);


        for (size_t i = 0 ; i < frames.size() ; i++) {
            Transform model = Identity() * Translation(i*5, 0,0)  * RotationY(i * (360/frames.size()));
            Transform mvo = ortho * orthoView * model;
            location= glGetUniformLocation(m_program_shadow, "modelMatrix");
            glUniformMatrix4fv(location, 1, GL_TRUE, model.buffer());
            location= glGetUniformLocation(m_program_shadow, "mvpMatrix");
            glUniformMatrix4fv(location, 1, GL_TRUE, mvo.buffer());
            glDrawArrays(GL_TRIANGLES, frames[i] * 2 * m_objet.count , m_objet.count);
        }

        glUseProgram(m_program_shadow2);
        glBindVertexArray(m_objet2.vao);

        location= glGetUniformLocation(m_program_shadow2, "viewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, orthoView.buffer());
        location= glGetUniformLocation(m_program_shadow2, "invViewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, invOrthoView.buffer());
        location= glGetUniformLocation(m_program_shadow2, "lightPos");
        glUniform4f(location,250.f,50.f,0.f,1.f);

        Transform model = Identity();
        Transform mvo = ortho * orthoView * model;
        location= glGetUniformLocation(m_program_shadow2, "modelMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, model.buffer());
        location= glGetUniformLocation(m_program_shadow2, "mvpMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, mvo.buffer());
        glDrawArrays(GL_TRIANGLES, 0 , m_objet2.count);

        /*


        */

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glViewport(0,0, window_width(), window_height());

        glUseProgram(m_program);
        glBindVertexArray(m_objet.vao);

        location= glGetUniformLocation(m_program, "dt");
        glUniform1f(location, dt);
        location= glGetUniformLocation(m_program, "viewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, view.buffer());
        location= glGetUniformLocation(m_program, "invViewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, invView.buffer());
        location= glGetUniformLocation(m_program, "lightPos");
        glUniform4f(location,50.f,250.f,0.f,1.f);
        location= glGetUniformLocation(m_program, "lightColor");
        glUniform4f(location,1.f,1.f,1.f,1.f);
        // dessiner les triangles de l'objet
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_objet.materials_buffer);

        glBindTexture(GL_TEXTURE_2D, shadow_map);

        location= glGetUniformLocation(m_program, "shadowMap");
        glUniform1i(location, 0);   // utilise la texture selectionnee sur l'unite 0


        for (size_t i = 0 ; i < frames.size() ; i++) {
            Transform model = Identity() * Translation(i*5, 0,0)  * RotationY(i * (360/frames.size()));
            Transform mvp = projection * view * model;
            Transform mvo = ortho * orthoView * model;
            location= glGetUniformLocation(m_program, "modelMatrix");
            glUniformMatrix4fv(location, 1, GL_TRUE, model.buffer());
            location= glGetUniformLocation(m_program, "mvpMatrix");
            glUniformMatrix4fv(location, 1, GL_TRUE, mvp.buffer());
            location= glGetUniformLocation(m_program, "sourceMatrix");
            glUniformMatrix4fv(location, 1, GL_TRUE, (Viewport(1,1) * mvo).buffer());
            glDrawArrays(GL_TRIANGLES, frames[i] * 2 * m_objet.count , m_objet.count);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(m_program2);
        glBindVertexArray(m_objet2.vao);

        location= glGetUniformLocation(m_program2, "viewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, view.buffer());
        location= glGetUniformLocation(m_program2, "invViewMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, invView.buffer());
        location= glGetUniformLocation(m_program2, "lightPos");
        glUniform4f(location,250.f,50.f,0.f,1.f);
        location= glGetUniformLocation(m_program2, "lightColor");
        glUniform4f(location,1.f,1.f,1.f,1.f);
        // dessiner les triangles de l'objet
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_objet2.materials_buffer);

        model = Identity();
        Transform mvp = projection * view * model;
        Transform mvo2 = ortho * orthoView * model;
        location= glGetUniformLocation(m_program2, "modelMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, model.buffer());
        location= glGetUniformLocation(m_program2, "mvpMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, mvp.buffer());

        glBindTexture(GL_TEXTURE_2D, shadow_map);

        location= glGetUniformLocation(m_program2, "shadowMap");
        glUniform1i(location, 0);   // utilise la texture selectionnee sur l'unite 0

        location= glGetUniformLocation(m_program2, "sourceMatrix");
        glUniformMatrix4fv(location, 1, GL_TRUE, (Viewport(1,1) * mvo2).buffer());

        glDrawArrays(GL_TRIANGLES, 0 , m_objet2.count);
        glBindTexture(GL_TEXTURE_2D, 0);
        return 1;
    }

protected:
    std::vector<Mesh> m_meshes;
    Mesh m_floor;
    Buffers m_objet, m_objet2;
    GLuint m_program, m_program2;
    GLuint m_program_shadow, m_program_shadow2;
    GLuint framebuffer;
    GLuint color_texture;
    GLuint shadow_map;
    Orbiter m_camera;
    std::vector<int> frames ;
    float t0,t1,dt;
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer
    TP tp;
    tp.run();

    return 0;
}
