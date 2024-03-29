#define N_RAY 1024

#include <cfloat>
#include <random>
#include <chrono>

#include "vec.h"
#include "mesh.h"
#include "wavefront.h"
#include "orbiter.h"

#include "image.h"
#include "image_io.h"
#include "image_hdr.h"


struct Ray
{
    Point o;
    float pad;
    Vector d;
    float tmax;
    
    Ray( ) : o(), d(), tmax(0) {}
    Ray( const Point& _o, const Point& _e ) : o(_o), d(Vector(_o, _e)), tmax(1) {}
    Ray( const Point& _o, const Vector& _d ) : o(_o), d(_d), tmax(FLT_MAX) {}
};


// intersection rayon / triangle.
struct Hit
{
    int triangle_id;
    float t;
    float u, v;
    
    Hit( ) : triangle_id(-1), t(0), u(0), v(0) {}       // pas d'intersection
    Hit( const int _id, const float _t, const float _u, const float _v ) : triangle_id(_id), t(_t), u(_u), v(_v) {}
    
    operator bool( ) const { return (triangle_id != -1); }      // renvoie vrai si l'intersection est initialisee...
};

// renvoie la normale interpolee d'un triangle.
Vector normal( const Hit& hit, const TriangleData& triangle )
{
    return normalize((1 - hit.u - hit.v) * Vector(triangle.na) + hit.u * Vector(triangle.nb) + hit.v * Vector(triangle.nc));
}

// renvoie le point d'intersection sur le triangle.
Point point( const Hit& hit, const TriangleData& triangle )
{
    return (1 - hit.u - hit.v) * Point(triangle.a) + hit.u * Point(triangle.b) + hit.v * Point(triangle.c);
}

// renvoie le point d'intersection sur le rayon
Point point( const Hit& hit, const Ray& ray )
{
    return ray.o + hit.t * ray.d;
}


// triangle "intersectable".
struct Triangle
{
    Point p;
    Vector e1, e2;
    int id;
    
    Triangle( const Point& _a, const Point& _b, const Point& _c, const int _id ) : p(_a), e1(Vector(_a, _b)), e2(Vector(_a, _c)), id(_id) {}
    
    /* calcule l'intersection ray/triangle
        cf "fast, minimum storage ray-triangle intersection" 
        http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
        
        renvoie faux s'il n'y a pas d'intersection valide (une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 htmax] du rayon.)
        renvoie vrai + les coordonnees barycentriques (u, v) du point d'intersection + sa position le long du rayon (t).
        convention barycentrique : p(u, v)= (1 - u - v) * a + u * b + v * c
    */
    Hit intersect( const Ray &ray, const float htmax ) const
    {
        Vector pvec= cross(ray.d, e2);
        float det= dot(e1, pvec);
        
        float inv_det= 1 / det;
        Vector tvec(p, ray.o);

        float u= dot(tvec, pvec) * inv_det;
        if(u < 0 || u > 1) return Hit();

        Vector qvec= cross(tvec, e1);
        float v= dot(ray.d, qvec) * inv_det;
        if(v < 0 || u + v > 1) return Hit();

        float t= dot(e2, qvec) * inv_det;
        if(t > htmax || t < 0) return Hit();
        
        return Hit(id, t, u, v);           // p(u, v)= (1 - u - v) * a + u * b + v * c
    }
};


// ensemble de triangles. 
// a remplacer par une vraie structure acceleratrice, un bvh, par exemple
struct BVH
{
    std::vector<Triangle> triangles;
    
    BVH( ) = default;
    BVH( const Mesh& mesh ) { build(mesh); }
    
    void build( const Mesh& mesh )
    {
        triangles.clear();
        triangles.reserve(mesh.triangle_count());
        for(int id= 0; id < mesh.triangle_count(); id++)
        {
            TriangleData data= mesh.triangle(id);
            triangles.push_back( Triangle(data.a, data.b, data.c, id) );
        }
        
        printf("%d triangles\n", int(triangles.size()));
        assert(triangles.size());
    }
    
    Hit intersect( const Ray& ray ) const
    {
        Hit hit;
        float tmax= ray.tmax;
        for(int id= 0; id < int(triangles.size()); id++)
            // ne renvoie vrai que si l'intersection existe dans l'intervalle [0 tmax]
            if(Hit h= triangles[id].intersect(ray, tmax))
            {
                hit= h;
                tmax= h.t;
            }
        
        return hit;        
    }
    
    bool visible( const Ray& ray ) const
    {
        for(int id= 0; id < int(triangles.size()); id++)
            if(triangles[id].intersect(ray, ray.tmax))
                return false;
        
        return true;
    }
};


struct Source
{
    Point a, b, c;
    Color emission;
    Vector n;
    float area;

    Source( ) : a(), b(), c(), emission(), n(), area() {}

    Source( const TriangleData& data, const Color& color ) : a(data.a), b(data.b), c(data.c), emission(color)
    {
       // normale geometrique du triangle abc, produit vectoriel des aretes ab et ac
        Vector ng= cross(Vector(a, b), Vector(a, c));
        n= normalize(ng);
        area= length(ng) / 2;
    }

    Point sample( const float u1, const float u2 ) const
    {
        // cf GI compemdium eq 18
        float r1= std::sqrt(u1);
        float alpha= 1 - r1;
        float beta= (1 - u2) * r1;
        float gamma= u2 * r1;
        return alpha*a + beta*b + gamma*c;
    }

    float pdf( const Point& p ) const
    {
        // todo : devrait renvoyer 0 pour les points a l'exterieur du triangle...
        return 1.f / area;
    }
};


struct Sources
{
    std::vector<Source> sources;
    float emission;     // emission totale des sources
    float area;         // aire totale des sources

    Sources( const Mesh& mesh ) : sources()
    {
        build(mesh);

        printf("%d sources\n", int(sources.size()));
        assert(sources.size());
    }

    void build( const Mesh& mesh )
    {
        area= 0;
        emission= 0;
        sources.clear();
        for(int id= 0; id < mesh.triangle_count(); id++)
        {
            const TriangleData& data= mesh.triangle(id);
            const Material& material= mesh.triangle_material(id);
            if(material.emission.power() > 0)
            {
                Source source(data, material.emission);
                emission= emission + source.area * source.emission.power();
                area= area + source.area;

                sources.push_back(source);
            }
        }
    }

    int size( ) const { return int(sources.size()); }
    const Source& operator() ( const int id ) const { return sources[id]; }
};


// utilitaires
// construit un repere ortho tbn, a partir d'un seul vecteur, la normale d'un point d'intersection, par exemple.
// permet de transformer un vecteur / une direction dans le repere du monde.

// cf "generating a consistently oriented tangent space" 
// http://people.compute.dtu.dk/jerf/papers/abstracts/onb.html
// cf "Building an Orthonormal Basis, Revisited", Pixar, 2017
// http://jcgt.org/published/0006/01/01/
struct World
{
    World( const Vector& _n ) : n(_n) 
    {
        float sign= std::copysign(1.0f, n.z);
        float a= -1.0f / (sign + n.z);
        float d= n.x * n.y * a;
        t= Vector(1.0f + sign * n.x * n.x * a, sign * d, -sign * n.x);
        b= Vector(d, sign + n.y * n.y * a, -n.y);        
    }
    
    // transforme le vecteur du repere local vers le repere du monde
    Vector operator( ) ( const Vector& local )  const { return local.x * t + local.y * b + local.z * n; }
    
    // transforme le vecteur du repere du monde vers le repere local
    Vector inverse( const Vector& global ) const { return Vector(dot(global, t), dot(global, b), dot(global, n)); }
    
    Vector t;
    Vector b;
    Vector n;
};

Color occlusion(const Color &mat, const BVH & bvh, const float &r1, const float &r2, const Vector &pn, const Point &p) {
    World wp(pn);

    float phi = 2 * M_PI * r1;

    float xd = std::cos(phi) * std::sqrt(1 - r2);
    float yd = std::sin(phi) * std::sqrt(1 - r2);
    float zd = std::sqrt(r2);

    Vector d(xd, yd, zd);

    Vector dworld = wp(d);

    float cos_theta = std::max(0.f, dot(pn, normalize(dworld)));
    Ray rayS(p + 0.001f * pn, dworld);

    return mat * bvh.visible(rayS);
}



int main( const int argc, const char **argv )
{
    const char *mesh_filename= "projet/data/cornell.obj";
    const char *orbiter_filename= "projet/data/cornell_orbiter.txt";
    
    if(argc > 1) mesh_filename= argv[1];
    if(argc > 2) orbiter_filename= argv[2];
    
    printf("%s: '%s' '%s'\n", argv[0], mesh_filename, orbiter_filename);
    
    // creer l'image resultat
    Image image(1024, 640);
    
    // charger un objet
    Mesh mesh= read_mesh(mesh_filename);
    if(mesh.triangle_count() == 0)
        // erreur de chargement, pas de triangles
        return 1;
    
    // creer l'ensemble de triangles / structure acceleratrice
    BVH bvh(mesh);
    Sources sources(mesh);
    
    // charger la camera
    Orbiter camera;
    if(camera.read_orbiter(orbiter_filename))
        // erreur, pas de camera
        return 1;
    
    // recupere les transformations view, projection et viewport pour generer les rayons
    Transform model= Identity();
    Transform view= camera.view();
    Transform projection= camera.projection(image.width(), image.height(), 45);
    Transform viewport= Viewport(image.width(), image.height());
    Transform invImg = Inverse(viewport * projection * view);

    auto cpu_start= std::chrono::high_resolution_clock::now();
    
    // parcourir tous les pixels de l'image
    // en parallele avec openMP, un thread par bloc de 16 lignes
#pragma omp parallel for schedule(dynamic, 1)
    for(int py= 0; py < image.height(); py++)
    {
        // nombres aleatoires, version c++11
        std::random_device seed;
        // un generateur par thread... pas de synchronisation
        std::default_random_engine rng(seed());
        // nombres aleatoires entre 0 et 1
        std::uniform_real_distribution<float> u01(0.f, 1.f);
        
        for(int px= 0; px < image.width(); px++)
        {
            Color true_color= Black();
            for (int j = 0 ; j < N_RAY ; j++){
                // generer le rayon pour le pixel (x, y)
                float x= px + u01(rng);
                float y= py + u01(rng);

                Point o = invImg(Point(x, y, 0)); // origine dans l'image
                Point e = invImg(Point(x, y, 1)); // extremite dans l'image

                Ray ray(o, e);
                // calculer les intersections
                if(Hit hit= bvh.intersect(ray))
                {
                    const TriangleData& triangle= mesh.triangle(hit.triangle_id);           // recuperer le triangle
                    const Material& material= mesh.triangle_material(hit.triangle_id);      // et sa matiere

                    Point p= point(hit, ray);               // point d'intersection
                    Vector pn= normal(hit, triangle);       // normale interpolee du triangle au point d'intersection

                    
                    // retourne la normale pour faire face a la camera / origine du rayon...
                    if(dot(pn, ray.d) > 0)
                        pn= -pn;

                    true_color = true_color + (1.f/N_RAY) * occlusion(material.diffuse, bvh, u01(rng), u01(rng), pn, p);

                    Color color= Black();
                    for (int i = 0; i < sources.sources.size() ; i++) {
                        float r1 = u01(rng);
                        float r2 = u01(rng);
                        Point esa = sources.sources[i].sample(r1,r2);
                        Vector sn = normalize(cross(sources.sources[i].b - sources.sources[i].a, sources.sources[i].c - sources.sources[i].a));
                        Ray rayS(p + 0.00001 * pn, esa + 0.00001 * sn);
                        if (bvh.visible(rayS)){
                            // accumuler la couleur de l'echantillon
                            if(dot(sn, rayS.d) > 0)
                                sn= -sn;
                            float cos_theta_p= std::max(0.f, dot(pn, normalize(rayS.d)));
                            float cos_theta_e= std::max(0.f, dot(sn, normalize(-rayS.d)));
                            float cos_theta = cos_theta_e * cos_theta_p;
                            Color contribution = (1.f / float(M_PI) * material.diffuse * cos_theta / distance2(esa, p));
                            Color contributionPond = (((contribution * sources.sources[i].emission)/sources.size()) / sources.sources[i].pdf(esa));
                            color= color + contributionPond;
                        }
                    }
                }
                //float gamma_tone = 2.2f;

                //true_color = Color(std::pow(true_color.r, (1.f/gamma_tone)), std::pow(true_color.g, (1.f/gamma_tone)), std::pow(true_color.b, (1.f/gamma_tone)), std::pow(true_color.a, (1.f/gamma_tone)));
            }
            
            image(px, py)= Color(true_color, 1);
        }
    }
    
    auto cpu_stop= std::chrono::high_resolution_clock::now();
    int cpu_time= std::chrono::duration_cast<std::chrono::milliseconds>(cpu_stop - cpu_start).count();
    printf("cpu  %ds %03dms\n", int(cpu_time / 1000), int(cpu_time % 1000));
    
    // enregistrer l'image resultat
    write_image(image, "render.png");
    write_image_hdr(image, "render.hdr");
    return 0;
}
