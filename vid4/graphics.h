#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "lib/obj_loader.h"

class Util
{
public:
	static std::string LoadShader(const std::string& filename)
	{
		std::ifstream file;
		file.open(filename.c_str());
		
		std::string output;
		std::string line;
		
		if (!file.is_open())
			throw std::runtime_error("File not opened.");
			
		while (file.good())
		{
			getline(file, line);
			output.append(line + "\n");
		}
		
		return output;
	}
	
	static void CheckShaderError(
		GLuint shader, 
		GLuint flag, 
		bool is_program, 
		const std::string& error_msg)
	{
		GLint success = 0;
		GLchar error[1024] = { 0 };
		
		if (is_program)
			glGetProgramiv(shader, flag, &success);
		else
			glGetShaderiv(shader, flag, &success);
		
		if (success == GL_FALSE)
		{
			if (is_program)
				glGetProgramInfoLog(shader, sizeof(error), NULL, error);
			else
				glGetShaderInfoLog(shader, sizeof(error), NULL, error);
			
			throw std::runtime_error("Shader or program error");
		}
	}
	
	static GLuint CreateShader(const std::string& text, GLenum shader_type)
	{
		GLuint shader = glCreateShader(shader_type);
		
		if (shader == 0)
			throw std::runtime_error("Shader creation failed");
		
		const GLchar* sources[1];
		GLint source_lengths[1];
		
		sources[0] = text.c_str();
		source_lengths[0] = text.length();
		
		glShaderSource(shader, 1, sources, source_lengths);
		glCompileShader(shader);
		
		Util::CheckShaderError(shader, GL_COMPILE_STATUS, false, 
			"Shader compilation failed."
		);
		
		return shader;
	}
};


class Camera
{
public:
	Camera(const glm::vec3& pos, float fov, float aspect, float near, float far)
	{
		this->pos = pos;
		//perspective = glm::perspective(fov, aspect, near, far);
		perspective = glm::ortho(-10.0f, 10.0f, -7.5f, 7.5f, 0.0001f, 1000.0f);
		forward = glm::vec3(0, 0, 1);
		up = glm::vec3(0, -1, 0);
	}
	
	inline glm::mat4 GetViewProjection() const
	{
		return perspective * glm::lookAt(pos, pos + forward, up);
	}
protected:
private:
	glm::mat4 perspective;
	glm::vec3 pos;
	glm::vec3 forward;
	glm::vec3 up;
};


class Transform
{
public:
	Transform(
		const glm::vec3& pos = glm::vec3(), 
		const glm::vec3& rot = glm::vec3(), 
		const glm::vec3& scale = glm::vec3(1.0f, 1.0f, 1.0f))
	{
		this->world = glm::vec3(0.0f, 0.0f, 0.0f);
		this->pos = pos;
		this->rot = rot;
		this->scale = scale;
	}
	
	inline glm::mat4 GetModel() const
	{
		glm::mat4 worldmat = glm::translate(world);
		glm::mat4 posmat = glm::translate(pos);
		
		glm::mat4 scalemat = glm::scale(scale);
		
		glm::mat4 rotxmat = glm::rotate(rot.x, glm::vec3(1,0,0));
		glm::mat4 rotymat = glm::rotate(rot.y, glm::vec3(0,1,0));
		glm::mat4 rotzmat = glm::rotate(rot.z, glm::vec3(0,0,1));
		glm::mat4 rotmat = rotzmat * rotymat * rotxmat;
		
		return posmat * worldmat * rotmat * scalemat;
	}
	
	inline glm::vec3& GetWorld() { return world; }
	inline glm::vec3& GetPos() { return pos; }
	inline glm::vec3& GetRot() { return rot; }
	inline glm::vec3& GetScale() { return scale; }
	
	inline void SetWorld(const glm::vec3& world) { this->world = world; }
	inline void SetPos(const glm::vec3& pos) { this->pos = pos; }
	inline void SetRot(const glm::vec3& rot) { this->rot = rot; }
	inline void SetScale(const glm::vec3& scale) { this->scale = scale; }
	
	inline float GetDrawOrder() const { return pos.y; }
protected:
private:
	glm::vec3 world;
	glm::vec3 pos;
	glm::vec3 rot;
	glm::vec3 scale;
};


class Shader
{
public:
	Shader(const std::string& filename)
	{
		program = glCreateProgram();
		
		shaders[0] = Util::CreateShader(
			Util::LoadShader(filename + ".vs"), 
			GL_VERTEX_SHADER
		);
		
		shaders[1] = Util::CreateShader(
			Util::LoadShader(filename + ".fs"),
			GL_FRAGMENT_SHADER
		);
		
		for (unsigned int i = 0; i < NUM_SHADERS; i++)
			glAttachShader(program, shaders[i]);
		
		glBindAttribLocation(program, 0, "position");
		glBindAttribLocation(program, 1, "texcoord");
		glBindAttribLocation(program, 2, "normal");
		
		glLinkProgram(program);
		Util::CheckShaderError(program, GL_LINK_STATUS, true, 
			"Shader program linking failed."
		);

		glValidateProgram(program);
		Util::CheckShaderError(program, GL_VALIDATE_STATUS, true, 
			"Shader program validation failed."
		);
		
		uniforms[TRANSFORM_U] = glGetUniformLocation(program, "transform");

		uniforms[SAMPLER0_U] = glGetUniformLocation(program, "sampler0");
		uniforms[SAMPLER1_U] = glGetUniformLocation(program, "sampler1");
		
		uniforms[TINT_U] = glGetUniformLocation(program, "tint");
		
		SetTint(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	}
	
	void Bind()
	{
		glUniform1i(uniforms[SAMPLER0_U], 0);
		glUniform1i(uniforms[SAMPLER1_U], 1);
		glUseProgram(program);
	}

	void Update(const Transform& transform, const Camera& camera)
	{
		glm::mat4 mvp = camera.GetViewProjection() * transform.GetModel();
		glUniformMatrix4fv(uniforms[TRANSFORM_U], 1, GL_FALSE, &mvp[0][0]);
		
		glUniform4f(uniforms[TINT_U], tint.x, tint.y, tint.z, tint.w);
	}
	
	virtual ~Shader()
	{
		for (unsigned int i = 0; i < NUM_SHADERS; i++)
		{
			glDetachShader(program, shaders[i]);
			glDeleteShader(shaders[i]);
		}
		
		glDeleteProgram(program);
	}
	
	inline glm::vec4 GetTint() const { return tint; }
	inline void SetTint(glm::vec4 tint) { this->tint = tint; }
protected:
private:
	static const unsigned int NUM_SHADERS = 2;

	enum
	{
		TRANSFORM_U,
		
		SAMPLER0_U,
		SAMPLER1_U,
		
		TINT_U,
		
		NUM_UNIFORMS
	};

	GLuint program;
	GLuint shaders[NUM_SHADERS];
	GLuint uniforms[NUM_UNIFORMS];
	
	glm::vec4 tint;
};


class Vertex
{
public:
	Vertex(
		const glm::vec3& pos, 
		const glm::vec2& texcoord, 
		const glm::vec3& normal = glm::vec3(0,0,0))
	{
		this->pos = pos;
		this->texcoord = texcoord;
		this->normal = normal;
	}
	
	inline glm::vec3* GetPos() { return &pos; }
	inline glm::vec2* GetTexCoord() { return &texcoord; }
	inline glm::vec3* GetNormal() { return &normal; }
	
protected:
private:
	glm::vec3 pos;
	glm::vec2 texcoord;
	glm::vec3 normal;
};


class Mesh
{
public:
	Mesh(
		Vertex* vertices, 
		unsigned int num_vertices, 
		unsigned int* indices, 
		unsigned int num_indices)
	{
		IndexedModel model;
		
		for (unsigned int i = 0; i < num_vertices; i++)
		{
			model.positions.push_back(*vertices[i].GetPos());
			model.texCoords.push_back(*vertices[i].GetTexCoord());
			model.normals.push_back(*vertices[i].GetNormal());
		}
		
		for (unsigned int i = 0; i < num_indices; i++)
		{
			model.indices.push_back(indices[i]);
		}
			
		init_mesh(model);
	}
	
	Mesh(const std::string& filename)
	{
		IndexedModel model = OBJModel(filename).ToIndexedModel();
		
		init_mesh(model);
	}
	
	void Draw()
	{
		glBindVertexArray(data);
		
		glDrawElements(GL_TRIANGLES, index_draw_count, GL_UNSIGNED_INT, 0);
		
		glBindVertexArray(0);
	}
	
	virtual ~Mesh()
	{
		glDeleteVertexArrays(1, &data);
	}
protected:
private:
	void init_mesh(const IndexedModel& model)
	{
		index_draw_count = model.indices.size();
		
		glGenVertexArrays(1, &data);
		glBindVertexArray(data);
		
		glGenBuffers(NUM_BUFFERS, buffers);
		
		// Position
		glBindBuffer(GL_ARRAY_BUFFER, buffers[POSITION_VB]);
		glBufferData(
			GL_ARRAY_BUFFER, 
			model.positions.size() * sizeof(model.positions[0]), 
			&model.positions[0],
			GL_STATIC_DRAW
		);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		
		// Texcoords
		glBindBuffer(GL_ARRAY_BUFFER, buffers[TEXCOORD_VB]);
		glBufferData(
			GL_ARRAY_BUFFER, 
			model.texCoords.size() * sizeof(model.texCoords[0]), 
			&model.texCoords[0],
			GL_STATIC_DRAW
		);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		
		// Normals
		glBindBuffer(GL_ARRAY_BUFFER, buffers[NORMAL_VB]);
		glBufferData(
			GL_ARRAY_BUFFER, 
			model.normals.size() * sizeof(model.normals[0]), 
			&model.normals[0],
			GL_STATIC_DRAW
		);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		
		// Indices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[INDEX_VB]);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER, 
			model.indices.size() * sizeof(model.indices[0]), 
			&model.indices[0],
			GL_STATIC_DRAW
		);
		
		glBindVertexArray(0);
	}

	enum
	{
		POSITION_VB,
		TEXCOORD_VB,
		NORMAL_VB,
		
		INDEX_VB,
		
		NUM_BUFFERS
	};
	
	GLuint data;
	GLuint buffers[NUM_BUFFERS];
	unsigned int index_draw_count;
};


class Texture
{
public:
	Texture(const std::string& filename)
	{
		SDL_Surface* surf = IMG_Load(("res/" + filename).c_str());
		
		if (surf == NULL)
			throw std::runtime_error("Texture file load failed");
		
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		
		
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			surf->w,
			surf->h,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			surf->pixels
		);
		
		width = surf->w;
		height = surf->h;
		
		SDL_FreeSurface(surf);
	}
	
	void Bind(unsigned int unit)
	{
		assert(unit >= 0 && unit <= 31);
		
		glActiveTexture(GL_TEXTURE0 + unit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		glActiveTexture(GL_TEXTURE0);
	}
	
	virtual ~Texture()
	{
		glDeleteTextures(1, &texture);
	}
	
	inline int GetWidth() const { return width; }
	inline int GetHeight() const { return height; }
	
	inline Point GetOffset() const { return offset; }
	inline void SetOffset(Point offset) { this->offset = offset; }
protected:
private:
	GLuint texture;
	int width;
	int height;
	Point offset;
};


class AssetManager
{
public:
	AssetManager()
	{
	}
	
	virtual ~AssetManager() 
	{
		typedef std::map<std::string,Shader*>::iterator it_shader;
		for(it_shader iter = shaders.begin(); iter != shaders.end(); iter++)
			delete iter->second;
			
		typedef std::map<std::string,Mesh*>::iterator it_mesh;
		for(it_mesh iter = meshes.begin(); iter != meshes.end(); iter++)
			delete iter->second;
			
		typedef std::map<std::string,Texture*>::iterator it_texture;
		for(it_texture iter = textures.begin(); iter != textures.end(); iter++)
			delete iter->second;
	}
	
	Shader* GetShader(const std::string& shader_name)
	{
		if (shaders.find(shader_name) == shaders.end()) 
		{
			// Shader not found, try to create it.
			shaders.insert(std::make_pair(
				shader_name, 
				new Shader(shader_name)
			));
		}
		
		return shaders[shader_name];
	}
	
	Mesh* GetMesh(const std::string& mesh_name)
	{
		if (meshes.find(mesh_name) == meshes.end())
		{
			// Mesh not found.
			throw std::runtime_error("Mesh not found.");
		}
		
		return meshes[mesh_name];
	}
	
	Texture* GetTexture(const std::string& texture_name)
	{
		if (textures.find(texture_name) == textures.end())
		{
			// Texture not found, try to load/register it.
			textures.insert(std::make_pair(
				texture_name,
				new Texture(texture_name)
			));
		}
		
		return textures[texture_name];
	}
	
	void RegisterMesh(const std::string& mesh_name, Mesh* mesh)
	{	
		meshes.insert(std::make_pair(mesh_name, mesh));
	}
protected:
private:
	std::map<std::string,Shader*> shaders;
	std::map<std::string,Mesh*> meshes;
	std::map<std::string,Texture*> textures;
};


class Component
{
public:
	Component(AssetManager& asset_manager)
	{
		this->asset_manager = &asset_manager;
	}
	
	virtual void Update() {}
	virtual void Draw(const Camera& camera) {}
	
	virtual ~Component() {}
	
	inline Transform& GetTransform() { return transform; }
    
    inline float GetDrawOrder() const
    {
    	const float ret = transform.GetDrawOrder();
    	return ret;
    }
protected:
	AssetManager* asset_manager;
	Transform transform;
private:
};


class YetiComponent : public Component
{
public:
	YetiComponent(AssetManager& asset_manager, 
				  Character& character,
				  const std::string& skin = "yeti")
		: Component(asset_manager),
		  character(character)
	{
		shader = asset_manager.GetShader("shader");
		texture = asset_manager.GetTexture(skin + ".png");
		
		Point draw_offset = texture->GetOffset();
		
		float w = (float)texture->GetWidth() / (float)TILE_SIZE;
		float h = (float)texture->GetHeight() / (float)TILE_SIZE;
		
		float x = w / 2.0f;
		float y = h / 2.0f;
		
		// Offsets
		float ox = 0;
		float oy = 0;
		
		if (draw_offset.x != 0)
			ox = (float)draw_offset.x / (float)TILE_SIZE;
		
		if (draw_offset.y != 0)
			oy = (float)draw_offset.y / (float)TILE_SIZE;
		
		Vertex vertices[] = {
			Vertex(glm::vec3(-x - ox,-y - oy, 0.0),glm::vec2(0.0, 0.0)),
			Vertex(glm::vec3(-x - ox, y - oy, 0.0),glm::vec2(0.0, 1.0)),
			Vertex(glm::vec3( x - ox, y - oy, 0.0),glm::vec2(1.0, 1.0)),
			Vertex(glm::vec3( x - ox,-y - oy, 0.0),glm::vec2(1.0, 0.0)),
		};
	
		unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
	
		mesh = new Mesh(
			vertices, 
			sizeof(vertices) / sizeof(vertices[0]),
			indices,
			sizeof(indices) / sizeof(indices[0])
		);
	}
	
	virtual ~YetiComponent()
	{
		delete mesh;
	}
	
	void Update()
	{
		static bool first_update = true;
		
		
		tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		
		if (character.GetAction() != NULL)
		{
			tint.z = 1.0f;
			tint.w = 0.5f;
		}
		
		if (character.GetAffectedBy().size() > 0)
		{
			tint.y = 1.0f;
			tint.w = 0.5f;
		}
		
		if (character.GetHP() < 100)
		{
			tint.r = (float)(((character.GetHP() - 50) * -1 + 50) / 100.0f);
			tint.w = 0.5f;
		}
		
		
		
		if (!character.IsMoving() && !first_update)
			return;
		
		first_update = false;

		unsigned int delta = Time::GetNow() - character.GetLastMove();
		float interpolation = (float)delta / (float)character.GetSpeed();
		
		Point direction = character.DirectionMoving();
		
		float x_interpolation = (float)direction.x * interpolation;
		float y_interpolation = (float)direction.y * interpolation;
		
		glm::vec3 new_pos(
			(float)character.GetLoc().x + x_interpolation + 0.0f,
			(float)character.GetLoc().y + y_interpolation + 0.0f,
			0.0f
		);
		
		GetTransform().SetPos(new_pos);
	}
	
	void Draw(const Camera& camera)
	{
		shader->SetTint(tint);
		shader->Bind();
		shader->Update(transform, camera);
		texture->Bind(0);
		mesh->Draw();
	}
	
	inline Character& GetCharacter() { return character; }
protected:
private:
	Shader* shader;
	Mesh* mesh;
	Texture* texture;
	Character& character;
	glm::vec4 tint;
};


class StaticSpriteComponent : public Component
{
public:
	StaticSpriteComponent(
		AssetManager& asset_manager,
		const std::string& shader_name,
		const std::string& mesh_name,
		const std::string& texture_name
	) 
		: Component(asset_manager)
	{
		shader = asset_manager.GetShader(shader_name);
		mesh = asset_manager.GetMesh(mesh_name);
		texture = asset_manager.GetTexture(texture_name);
	}

	void Draw(const Camera& camera)
	{
		shader->Bind();
		shader->Update(transform, camera);
		texture->Bind(0);
		mesh->Draw();
	}
protected:
	Shader* shader;
	Mesh* mesh;
	Texture* texture;
private:
};


class TileBorderComponent : public StaticSpriteComponent
{
public:
	TileBorderComponent(AssetManager& asset_manager)
		: StaticSpriteComponent(
			asset_manager, 
			"shader", 
			"square", 
			"tile_border.png"
		)
	{
	}
protected:
private:
};


struct ComponentDrawOrderer
{
	bool operator()(Component* lc, Component* rc) {
		return lc->GetDrawOrder() < rc->GetDrawOrder();
	}
};


class GraphicsEngine
{
public:
	GraphicsEngine(GameEngine& game_engine)
		: game_engine(game_engine)
	{
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		window = SDL_CreateWindow(
			"greetings", 
			1120,//SDL_WINDOWPOS_CENTERED,
			560,//SDL_WINDOWPOS_CENTERED,
			800,
			600,
			SDL_WINDOW_OPENGL
		);
		
		glcontext = SDL_GL_CreateContext(window);
		
		GLenum status = glewInit();
		if (status != GLEW_OK)
			throw std::runtime_error("Glew failed to initialize.");
		
		glDisable(GL_DEPTH_TEST);
		
		
		camera = new Camera(
			glm::vec3(0,0,-3), 70.0f, 800.0f/600.0f, 0.01f, 1000.0f
		);
	}
	
	virtual ~GraphicsEngine()
	{
		for (unsigned int i = 0; i < components.size(); i++)
			delete components[i];
			
		delete camera;
		
		SDL_GL_DeleteContext(glcontext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
	
	void Register(Component* component)
	{
		components.push_back(component);
	}
	
	void Deregister(Component* component)
	{
		unsigned int offset = 0;
		for (auto &c : components) {
			if (c == component)
				goto found;
			offset++;
		}
		throw std::runtime_error("Component not found.");
	found:
		components.erase(components.begin() + offset);
	}
	
	void Draw()
	{
		for (auto &c : components)
			c->Update();
		
		if (Character* avatar = game_engine.GetAvatar())
		{
			glm::vec3 avatar_pos = FindComponent(avatar)->
								   GetTransform().
								   GetPos();
		
			world_transform.x = -1.0f * avatar_pos.x;
			world_transform.y = -1.0f * avatar_pos.y;
		}
				
		for (auto &i : components)
			i->GetTransform().SetWorld(world_transform);


		glClearColor(0.0f, 0.15f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		glLoadIdentity();
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
		std::sort(components.begin(), components.end(), ComponentDrawOrderer());
		
		for (auto &i : components)
			i->Draw(*camera);
		
		SDL_GL_SwapWindow(window);
	}
	
	Component* FindComponent(Entity* entity)
	{
		for (auto &c : components)
			if (YetiComponent* y = dynamic_cast<YetiComponent*>(c))
				if (&y->GetCharacter() == entity)
					return y;
	}
protected:
private:
	GameEngine& game_engine;
	std::vector<Component*> components;
	glm::vec3 world_transform;
	Camera* camera;
	SDL_Window* window;
	SDL_GLContext glcontext;
};

#endif
