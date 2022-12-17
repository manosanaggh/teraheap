#include <parallax_db.hpp>

std::vector<par_handle> dbs;

int Parallax_init(){
		const char *pathname = PATH;

		par_db_options db_options;
		db_options.volume_name = (char *)pathname;
		db_options.create_flag = PAR_CREATE_DB;
		db_options.options = par_get_default_options();
		dbs.clear();
		for (int i = 0; i < DB_NUM; ++i) {
			std::string db_name = "data" + std::to_string(i) + ".dat";
			db_options.db_name = (char *)db_name.c_str();
			const char *error_message = nullptr;
      std::cerr << "b4 par_open" << std::endl; 
			par_handle hd = par_open(&db_options, &error_message);
      std::cerr << "after par_open" << std::endl;

			if (error_message != nullptr) {
				std::cerr << error_message << std::endl;
				return 1;
			}

			dbs.push_back(hd);
		}

    return 0;
}

int Parallax_read(const std::string &key){
		std::hash<std::string> hash_fn;
		uint32_t db_id = hash_fn(key) % DB_NUM;
		std::map<std::string, std::string> vmap;
		struct par_key lookup_key = { .size = (uint32_t)key.length(), .data = (const char *)key.c_str() };
		struct par_value lookup_value = { .val_buffer = NULL };

		const char *error_message = NULL;
		par_get(dbs[db_id], &lookup_key, &lookup_value, &error_message);
		if (error_message) {
			std::cout << "[1]cannot find : " << key << " in DB " << db_id << std::endl;
			return 1;
		}
		free(lookup_value.val_buffer);
		lookup_value.val_buffer = NULL;

    return 0;
}

int Parallax_insert(const std::string &key, const std::string &value){
		std::hash<std::string> hash_fn;
		uint32_t db_id = hash_fn(key) % DB_NUM;
    return 0;
}
