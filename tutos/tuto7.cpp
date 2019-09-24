
//! \file tuto7.cpp reprise de tuto6.cpp mais en derivant App::init(), App::quit() et bien sur App::render().

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"
#include "program.h"
#include "uniforms.h"
#include "app_time.h"        // classe Application a deriver
struct Buffers
{
    GLuint vao;
    GLuint buffer;
    int count;

    Buffers( ) : vao(0),buffer(0), count(0) {}

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
        size_t stride = sizeof(vec3) + sizeof(vec2);



        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        glVertexAttribPointer(0,
            3, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            (const GLvoid *)0               // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        glBufferSubData(GL_ARRAY_BUFFER, offset,  mesh.vertex_buffer_size(), mesh.vertex_buffer());

        offset = offset + size;
        //offset = size;
        size= mesh.texcoord_buffer_size();

        // et configure l'attribut 1, vec2 texcoord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, /* stride */ 0, (const GLvoid *) offset);
        glEnableVertexAttribArray(1);

        glBufferSubData(GL_ARRAY_BUFFER, offset, mesh.texcoord_buffer_size(), mesh.texcoord_buffer());

        // conserve le nombre de sommets
        count= mesh.vertex_count();
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
        m_mesh= read_mesh("data/cube.obj");
        if(!m_mesh.vertex_count()) return -1;

        m_objet.create(m_mesh);
        
        Point pmin, pmax;
        m_mesh.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);

        m_texture= read_texture(0, "data/debug2x2red.png");

        m_program = read_program("tutos/tuto9_texture1.glsl");
        program_print_errors(m_program);

        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        return 0;   // ras, pas d'erreur
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        m_objet.release();
        glDeleteTextures(1, &m_texture);
        release_program(m_program);
        
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
        else if(key_state('a')){
            clear_key_state('a');
            m_nbobjets++;
            m_camera.lookat(Point(-m_nbobjets/2-0.5,0,-m_nbobjets/2-0.5), Point(m_nbobjets/2,0+0.5,m_nbobjets/2+0.5));
        }

        Transform view = m_camera.view();
        Transform projection = m_camera.projection(window_width(), window_height(), 45);

        Transform model;
        glUseProgram(m_program);
        int location;
        for(int i = 0; i < m_nbobjets; i++){
            for(int j = 0; j < m_nbobjets; j++){
                model = Translation(i-m_nbobjets/2,0,j-m_nbobjets/2);
                Transform mvp = projection * view * model;
                location= glGetUniformLocation(m_program, "mvpMatrix");
                glUniformMatrix4fv(location, 1, GL_TRUE, mvp.buffer());
                glBindVertexArray(m_objet.vao);
                location = glGetUniformLocation(m_program, "texture0");
                glUniform1i(location, 0);
                glBindTexture(GL_TEXTURE_2D, m_texture);
                // dessiner les triangles de l'objet
                glDrawArrays(GL_TRIANGLES, 0, m_objet.count);
            }
        }

        return 1;
    }

protected:
    Mesh m_mesh;
    Buffers m_objet;
    GLuint m_texture;
    GLuint m_program;
    Orbiter m_camera;
    int m_nbobjets = 1;
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}
